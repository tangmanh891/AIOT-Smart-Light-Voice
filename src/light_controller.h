#pragma once

#include <Arduino.h>

#include "config.h"

namespace LightController {
using StateReporter = void (*)(bool state);

void begin();
bool isOn();
void setState(Source source, bool on);
void blinkConfirm(unsigned long duration_ms);
void setStateReporter(StateReporter reporter);
}  // namespace LightController
