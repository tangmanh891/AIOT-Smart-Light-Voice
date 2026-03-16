#pragma once

#ifndef EIDSP_QUANTIZE_FILTERBANK
#define EIDSP_QUANTIZE_FILTERBANK 0
#endif

#include <Arduino.h>
#include "driver/i2s.h"

namespace Pins {
constexpr uint8_t kLed = 5;
constexpr uint8_t kButton = 18;
constexpr int kI2sWs = 25;
constexpr int kI2sSd = 22;
constexpr int kI2sSck = 26;
constexpr i2s_port_t kI2sPort = I2S_NUM_0;
}  // namespace Pins

namespace Thresholds {
constexpr float kMinWakeConf = 0.7f;
constexpr float kMinCmdConf = 0.8f;
constexpr float kImLangProbMin = 0.70f;
constexpr int kImLangMaxFrames = 10;
constexpr uint8_t kCmdConfirmFrames = 1;
}  // namespace Thresholds

namespace Timings {
constexpr unsigned long kActionCooldownMs = 800;
constexpr unsigned long kPostCommandDelayMs = 500;
constexpr unsigned long kLedCooldownMs = 600;
constexpr unsigned long kDebounceDelayMs = 50;
}  // namespace Timings

enum class RunState : uint8_t { kIdle, kAwake };
enum class Source : uint8_t { kButton, kSinric, kVoice, kTimer };
