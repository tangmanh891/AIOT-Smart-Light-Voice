#include "button_controller.h"

#include <Arduino.h>

#include "config.h"
#include "light_controller.h"

namespace {
bool g_initialized = false;
int g_last_stable_state = HIGH;
int g_last_reading = HIGH;
unsigned long g_last_debounce_time = 0;
}  // namespace

namespace ButtonController {

void begin() {
  pinMode(Pins::kButton, INPUT_PULLUP);
}

void loop() {
  if (!g_initialized) {
    g_last_stable_state = digitalRead(Pins::kButton);
    g_last_reading = g_last_stable_state;
    g_initialized = true;
    return;
  }

  const int reading = digitalRead(Pins::kButton);
  if (reading != g_last_reading) {
    g_last_debounce_time = millis();
  }
  g_last_reading = reading;

  if (millis() - g_last_debounce_time <= Timings::kDebounceDelayMs) {
    return;
  }

  if (reading == g_last_stable_state) {
    return;
  }

  g_last_stable_state = reading;
  if (g_last_stable_state == LOW) {
    LightController::setState(Source::kButton, !LightController::isOn());
    Serial.println("[BUTTON] Pressed");
  } else {
    Serial.println("[BUTTON] Released");
  }
}

}  // namespace ButtonController
