#include "audio_capture.h"

#include <cstdint>

#include <ESP32_INMP441_inferencing.h>
#include "driver/i2s.h"

#include "config.h"

namespace {
struct InferenceBuffer {
  int16_t* buffer = nullptr;
  uint8_t ready = 0;
  uint32_t count = 0;
  uint32_t total_samples = 0;
};

InferenceBuffer g_inference;
constexpr uint32_t kSampleBufferSize = 2048;
int16_t g_sample_buffer[kSampleBufferSize];
bool g_record_status = true;

void audioInferenceCallback(uint32_t n_bytes) {
  for (int i = 0; i < static_cast<int>(n_bytes >> 1); i++) {
    g_inference.buffer[g_inference.count++] = g_sample_buffer[i];
    if (g_inference.count >= g_inference.total_samples) {
      g_inference.count = 0;
      g_inference.ready = 1;
    }
  }
}

void captureSamples(void* arg) {
  const int32_t bytes_to_read =
      static_cast<int32_t>(reinterpret_cast<uintptr_t>(arg));
  size_t bytes_read = bytes_to_read;
  while (g_record_status) {
    i2s_read(Pins::kI2sPort, static_cast<void*>(g_sample_buffer), bytes_to_read,
             &bytes_read, 100);
    if (g_record_status) {
      audioInferenceCallback(bytes_to_read);
    } else {
      break;
    }
  }

  vTaskDelete(nullptr);
}

int initI2s(uint32_t sampling_rate) {
  const i2s_config_t cfg = {
      .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = static_cast<int>(sampling_rate),
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = I2S_COMM_FORMAT_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = 512,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = -1,
  };

  const i2s_pin_config_t pins = {
      .bck_io_num = Pins::kI2sSck,
      .ws_io_num = Pins::kI2sWs,
      .data_out_num = -1,
      .data_in_num = Pins::kI2sSd,
  };

  esp_err_t ret = i2s_driver_install(Pins::kI2sPort, &cfg, 0, nullptr);
  if (ret != ESP_OK) {
    Serial.printf("[ERROR] i2s_driver_install failed: %d\n", ret);
    return ret;
  }

  ret = i2s_set_pin(Pins::kI2sPort, &pins);
  if (ret != ESP_OK) {
    Serial.printf("[ERROR] i2s_set_pin failed: %d\n", ret);
    return ret;
  }

  i2s_zero_dma_buffer(Pins::kI2sPort);
  return ESP_OK;
}

}  // namespace

namespace AudioCapture {

bool begin(uint32_t sample_count) {
  g_inference.buffer = static_cast<int16_t*>(malloc(sample_count * sizeof(int16_t)));
  if (g_inference.buffer == nullptr) {
    return false;
  }

  g_inference.count = 0;
  g_inference.total_samples = sample_count;
  g_inference.ready = 0;

  if (initI2s(EI_CLASSIFIER_FREQUENCY) != ESP_OK) {
    ei_printf("Failed to start I2S!\n");
  }

  ei_sleep(100);
  g_record_status = true;
  xTaskCreate(captureSamples, "CaptureSamples", 1024 * 32,
              reinterpret_cast<void*>(static_cast<uintptr_t>(kSampleBufferSize)),
              10, nullptr);
  return true;
}

bool recordWindow(PollFn poll_fn) {
  uint32_t loop_count = 0;
  while (g_inference.ready == 0) {
    if (poll_fn != nullptr) {
      poll_fn();
    }

    delay(5);
    loop_count++;
    if (loop_count > 2000) {
      Serial.println("[WARN] Inference timeout");
      return false;
    }
  }

  g_inference.ready = 0;
  return true;
}

int getSignalData(size_t offset, size_t length, float* out_ptr) {
  numpy::int16_to_float(&g_inference.buffer[offset], out_ptr, length);
  return 0;
}

void end() {
  g_record_status = false;
  ei_sleep(100);
  free(g_inference.buffer);
  g_inference.buffer = nullptr;
}

}  // namespace AudioCapture
