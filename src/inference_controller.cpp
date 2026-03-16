#include "inference_controller.h"

#include <cstring>

#include <ESP32_INMP441_inferencing.h>

#include "config.h"
#include "light_controller.h"

namespace {
RunState g_state = RunState::kIdle;
unsigned long g_last_action_ms = 0;
uint8_t g_confirm_on = 0;
uint8_t g_confirm_off = 0;
int g_im_lang_count = 0;
bool g_debug_nn = false;

float getProb(const ei_impulse_result_t& res, const char* label) {
  for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    if (strcmp(res.classification[i].label, label) == 0) {
      return res.classification[i].value;
    }
  }

  return 0.0f;
}
}  // namespace

namespace InferenceController {

void begin() {
  ei_printf("Edge Impulse Wake->Command + SinricPro integration\n");
  if (!AudioCapture::begin(EI_CLASSIFIER_RAW_SAMPLE_COUNT)) {
    ei_printf("ERR: Could not allocate audio buffer\n");
    return;
  }

  ei_printf("Recording...\n");
}

void loop(AudioCapture::PollFn poll_fn) {
  if (!AudioCapture::recordWindow(poll_fn)) {
    return;
  }

  signal_t signal;
  signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
  signal.get_data = &AudioCapture::getSignalData;

  ei_impulse_result_t result = {0};
  const EI_IMPULSE_ERROR status = run_classifier(&signal, &result, g_debug_nn);
  if (status != EI_IMPULSE_OK) {
    return;
  }

  const unsigned long now_ms = millis();
  const float p_wake = getProb(result, "hello_den");
  const float p_on = getProb(result, "bat_den");
  const float p_off = getProb(result, "tat_den");
  const float p_im = getProb(result, "im_lang");
  const bool in_cooldown =
      static_cast<long>(now_ms - g_last_action_ms) < static_cast<long>(Timings::kActionCooldownMs);

  if (g_state == RunState::kIdle) {
    if (p_wake >= Thresholds::kMinWakeConf) {
      g_state = RunState::kAwake;
      g_confirm_on = 0;
      g_confirm_off = 0;
      g_im_lang_count = 0;

      LightController::blinkConfirm(100);
      Serial.println("[WAKE] hello_den -> AWAKE");
    }

    return;
  }

  Serial.printf(
      "p(hello_den)=%.2f  p(bat_den)=%.2f  p(tat_den)=%.2f p(unknown)=%.2f  p(im_lang)=%.2f\n",
      p_wake, p_on, p_off, getProb(result, "unknown"), p_im);

  if (p_im >= Thresholds::kImLangProbMin) {
    g_im_lang_count++;
  } else {
    g_im_lang_count = 0;
  }

  if (g_im_lang_count >= Thresholds::kImLangMaxFrames) {
    g_state = RunState::kIdle;
    g_im_lang_count = 0;
    Serial.println("[STATE] -> IDLE (10x im_lang)");
  }

  if (in_cooldown || g_state != RunState::kAwake) {
    return;
  }

  if (p_on >= Thresholds::kMinCmdConf) {
    g_confirm_on++;
  } else {
    g_confirm_on = 0;
  }

  if (p_off >= Thresholds::kMinCmdConf) {
    g_confirm_off++;
  } else {
    g_confirm_off = 0;
  }

  if (g_confirm_on >= Thresholds::kCmdConfirmFrames) {
    LightController::setState(Source::kVoice, true);
    g_last_action_ms = now_ms;
    g_confirm_on = 0;
    g_confirm_off = 0;
    delay(Timings::kPostCommandDelayMs);
  } else if (g_confirm_off >= Thresholds::kCmdConfirmFrames) {
    LightController::setState(Source::kVoice, false);
    g_last_action_ms = now_ms;
    g_confirm_on = 0;
    g_confirm_off = 0;
    delay(Timings::kPostCommandDelayMs);
  }
}

}  // namespace InferenceController
