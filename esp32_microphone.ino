
#define EIDSP_QUANTIZE_FILTERBANK  0

#include <WiFi.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>
#include <ESP32_INMP441_inferencing.h>
#include "driver/i2s.h"

/* -------------------- USER CONFIG -------------------- */
const char* WIFI_SSID = "TenWiFi";      // Thay bằng tên WiFi
const char* WIFI_PASS = "MatKhauWiFi";  // Thay bằng mật khẩu WiFi
const char* APP_KEY    = "your-app-key";
const char* APP_SECRET = "your-app-secret";
const char* DEVICE_ID  = "your-device-id";

/* -------------------- GPIO -------------------- */
#define LED_PIN      5
#define BUTTON_PIN   18
#define I2S_WS       25
#define I2S_SD       22
#define I2S_SCK      26
#define I2S_PORT     I2S_NUM_0

/* -------------------- MODEL THRESHOLDS -------------------- */
#define MIN_WAKE_CONF   0.7f
#define MIN_CMD_CONF    0.8f
#define IM_LANG_PROB_MIN 0.70f
#define IM_LANG_MAX_FRAMES 10
#define CMD_CONFIRM_FRAMES 1
#define ACTION_COOLDOWN_MS 800
#define POST_CMD_DELAY_MS 500

/* -------------------- SYSTEM STATES -------------------- */
enum RunState { STATE_IDLE, STATE_AWAKE };
static RunState run_state = STATE_IDLE;
static uint32_t last_action_ms = 0;
static uint8_t confirm_on = 0, confirm_off = 0;
static int im_lang_count = 0;

/* -------------------- LED + Sinric -------------------- */
enum Source { SRC_BUTTON, SRC_SINRIC, SRC_VOICE, SRC_TIMER };
volatile bool ledState = false; 
Source lastSrc = SRC_BUTTON;
unsigned long lastToggleMs = 0;
const unsigned long cooldownMs = 600;

bool onPowerState(const String &deviceId, bool &state);

/* -------------------- AUDIO -------------------- */
typedef struct {
  int16_t *buffer;
  uint8_t buf_ready;
  uint32_t buf_count;
  uint32_t n_samples;
} inference_t;

static inference_t inference;
static const uint32_t sample_buffer_size = 2048;
static int16_t sampleBuffer[sample_buffer_size];
static bool debug_nn = false;
static bool record_status = true;

/* -------------------- DECLARATIONS -------------------- */
static bool microphone_inference_start(uint32_t n_samples);
static bool microphone_inference_record(void);
static void microphone_inference_end(void);
static int  microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr);
static void audio_inference_callback(uint32_t n_bytes);
static void capture_samples(void* arg);
static int  i2s_init(uint32_t sampling_rate);
static int  i2s_deinit(void);
static float get_prob(const ei_impulse_result_t& res, const char* label);
void setLedBy(Source src, bool on);
void handleButton();

/* -------------------- SETUP -------------------- */
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);  
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("\n[WiFi] Connecting...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { 
    delay(400); 
    Serial.print("."); 
  }
  Serial.printf("\n[WiFi] Connected: %s | IP: %s\n", 
                WIFI_SSID, WiFi.localIP().toString().c_str());

  // SinricPro setup
  SinricProSwitch &sw = SinricPro[DEVICE_ID];
  sw.onPowerState(onPowerState);
  SinricPro.begin(APP_KEY, APP_SECRET);
  SinricPro.restoreDeviceStates(true);

  // Edge Impulse init
  ei_printf("Edge Impulse Wake->Command + SinricPro integration (FIXED)\n");
  if (!microphone_inference_start(EI_CLASSIFIER_RAW_SAMPLE_COUNT)) {
    ei_printf("ERR: Could not allocate audio buffer\n");
    return;
  }
  ei_printf("Recording...\n");
}

/* -------------------- LOOP -------------------- */
void loop() {
  SinricPro.handle();
  handleButton();

  if (!microphone_inference_record()) return;

  signal_t signal;
  signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
  signal.get_data = &microphone_audio_signal_get_data;

  ei_impulse_result_t result = { 0 };
  EI_IMPULSE_ERROR r = run_classifier(&signal, &result, debug_nn);
  if (r != EI_IMPULSE_OK) return;

  const uint32_t now_ms = millis();
  const float p_wake = get_prob(result, "hello_den");
  const float p_on   = get_prob(result, "bat_den");
  const float p_off  = get_prob(result, "tat_den");
  const float p_im   = get_prob(result, "im_lang");
  const bool in_cooldown = ((int32_t)(now_ms - last_action_ms) < (int32_t)ACTION_COOLDOWN_MS);

  // ---------------- STATE MACHINE ----------------
  if (run_state == STATE_IDLE) {
    if (p_wake >= MIN_WAKE_CONF) {
      run_state = STATE_AWAKE;
      confirm_on = confirm_off = 0;
      im_lang_count = 0;
      digitalWrite(LED_PIN, HIGH); 
      delay(100); 
      digitalWrite(LED_PIN, ledState ? HIGH : LOW);
      Serial.println("[WAKE] hello_den -> AWAKE");
    }
  } else { // STATE_AWAKE
    Serial.printf("p(hello_den)=%.2f  p(bat_den)=%.2f  p(tat_den)=%.2f p(unknown)=%.2f  p(im_lang)=%.2f\n",
      get_prob(result,"hello_den"),
      get_prob(result,"bat_den"),
      get_prob(result,"tat_den"),
      get_prob(result,"unknown"),
      get_prob(result,"im_lang"));
    
    if (p_im >= IM_LANG_PROB_MIN) im_lang_count++;
    else im_lang_count = 0;

    if (im_lang_count >= IM_LANG_MAX_FRAMES) {
      run_state = STATE_IDLE;
      im_lang_count = 0;
      Serial.println("[STATE] -> IDLE (10x im_lang)");
    }

    if (!in_cooldown && run_state == STATE_AWAKE) {
      if (p_on  >= MIN_CMD_CONF)  confirm_on++;
      else confirm_on = 0;  

      if (p_off >= MIN_CMD_CONF) confirm_off++;
      else confirm_off = 0; 

      if (confirm_on >= CMD_CONFIRM_FRAMES) {
        setLedBy(SRC_VOICE, true);
        last_action_ms = now_ms;
        confirm_on = confirm_off = 0;
        delay(POST_CMD_DELAY_MS);
      } else if (confirm_off >= CMD_CONFIRM_FRAMES) {
        setLedBy(SRC_VOICE, false);
        last_action_ms = now_ms;
        confirm_on = confirm_off = 0;
        delay(POST_CMD_DELAY_MS);
      }
    }
  }
}

/* -------------------- Sinric & LED -------------------- */
bool onPowerState(const String &deviceId, bool &state) {
  if (state == ledState) return true;
  setLedBy(SRC_SINRIC, state);
  return true;
}

void setLedBy(Source src, bool on) {
  unsigned long now = millis();
  if (now - lastToggleMs < cooldownMs) {
    Serial.println("[LED] Cooldown active, ignoring request");
    return;
  }

  bool changed = (ledState != on);
  ledState = on;
  digitalWrite(LED_PIN, on ? HIGH : LOW);
  lastSrc = src;
  lastToggleMs = now;

  Serial.printf("[LED] %s by %s\n", on ? "ON" : "OFF",
                src==SRC_BUTTON ? "BUTTON" :
                src==SRC_SINRIC ? "SINRIC" :
                src==SRC_VOICE  ? "VOICE"  : "TIMER");

  if (changed && src != SRC_SINRIC) {
    SinricProSwitch &sw = SinricPro[DEVICE_ID];
    sw.sendPowerStateEvent(ledState);
  }
}

/* -------------------- Button handler - IMPROVED -------------------- */
void handleButton() {
  static bool initialized = false;
  static int lastStableState;
  static int lastReading;
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 50;

  if (!initialized) {
    lastStableState = digitalRead(BUTTON_PIN);
    lastReading     = lastStableState;
    initialized = true;
    return;
  }

  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastReading) {
    lastDebounceTime = millis();
  }
  lastReading = reading;

  if ((millis() - lastDebounceTime) > debounceDelay) {

    if (reading != lastStableState) {

      lastStableState = reading;

      setLedBy(SRC_BUTTON, !ledState);

      if (lastStableState == LOW)
        Serial.println("[BUTTON] Pressed");
      else
        Serial.println("[BUTTON] Released");
    }
  }
}



/* -------------------- Helper functions -------------------- */
static float get_prob(const ei_impulse_result_t& res, const char* label) {
  for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    if (strcmp(res.classification[i].label, label) == 0) {
      return res.classification[i].value;
    }
  }
  return 0.0f;
}

/* -------------------- Audio -------------------- */
static void audio_inference_callback(uint32_t n_bytes) {
  for (int i = 0; i < (int)(n_bytes >> 1); i++) {
    inference.buffer[inference.buf_count++] = sampleBuffer[i];
    if (inference.buf_count >= inference.n_samples) {
      inference.buf_count = 0;
      inference.buf_ready = 1;
    }
  }
}

static void capture_samples(void* arg) {
  const int32_t bytes_to_read = (uint32_t)arg;
  size_t bytes_read = bytes_to_read;
  while (record_status) {
    i2s_read((i2s_port_t)I2S_PORT, (void*)sampleBuffer, bytes_to_read, &bytes_read, 100);
    if (record_status) audio_inference_callback(bytes_to_read);
    else break;
  }
  vTaskDelete(NULL);
}

static bool microphone_inference_start(uint32_t n_samples) {
  inference.buffer = (int16_t*)malloc(n_samples * sizeof(int16_t));
  if (inference.buffer == NULL) return false;
  inference.buf_count = 0;
  inference.n_samples = n_samples;
  inference.buf_ready = 0;

  if (i2s_init(EI_CLASSIFIER_FREQUENCY)) {
    ei_printf("Failed to start I2S!\n");
  }

  ei_sleep(100);
  record_status = true;
  xTaskCreate(capture_samples, "CaptureSamples", 1024*32, (void*)sample_buffer_size, 10, NULL);
  return true;
}

static bool microphone_inference_record(void) {
  uint32_t loop_count = 0;
  while (inference.buf_ready == 0) {
    if (loop_count % 2 == 0) { 
      SinricPro.handle();
    }
    handleButton();
    delay(5);
    loop_count++;
    
    if (loop_count > 2000) {  
      Serial.println("[WARN] Inference timeout");
      return false;
    }
  }
  inference.buf_ready = 0;
  return true;
}

static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr) {
  numpy::int16_to_float(&inference.buffer[offset], out_ptr, length);
  return 0;
}

static int i2s_init(uint32_t sampling_rate) {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = (int)sampling_rate,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, 
    .dma_buf_count = 8,
    .dma_buf_len = 512,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = -1
  };

  i2s_pin_config_t pins = {
    .bck_io_num = I2S_SCK,
    .ws_io_num  = I2S_WS,
    .data_out_num = -1,
    .data_in_num  = I2S_SD
  };

  esp_err_t ret = i2s_driver_install((i2s_port_t)I2S_PORT, &cfg, 0, NULL);
  if (ret != ESP_OK) {
    Serial.printf("[ERROR] i2s_driver_install failed: %d\n", ret);
    return ret;
  }
  
  ret = i2s_set_pin((i2s_port_t)I2S_PORT, &pins);
  if (ret != ESP_OK) {
    Serial.printf("[ERROR] i2s_set_pin failed: %d\n", ret);
    return ret;
  }
  
  i2s_zero_dma_buffer((i2s_port_t)I2S_PORT);
  return ESP_OK;
}

static int i2s_deinit(void) {
  i2s_driver_uninstall((i2s_port_t)I2S_PORT);
  return 0;
}

static void microphone_inference_end(void) {
  record_status = false;
  ei_sleep(100);
  free(inference.buffer);
}

/* -------------------- Guard -------------------- */
#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Invalid model for current sensor."
#endif