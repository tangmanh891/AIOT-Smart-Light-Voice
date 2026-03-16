#include "light_controller.h"

#include "config.h"

namespace {
bool g_led_state = false;
unsigned long g_last_toggle_ms = 0;
LightController::StateReporter g_state_reporter = nullptr;

const char* sourceToText(Source source) {
  switch (source) {
    case Source::kButton:
      return "BUTTON";
    case Source::kSinric:
      return "SINRIC";
    case Source::kVoice:
      return "VOICE";
    case Source::kTimer:
      return "TIMER";
  }
  return "UNKNOWN";
}
}  // namespace

namespace LightController {

void begin() {
  pinMode(Pins::kLed, OUTPUT);
  digitalWrite(Pins::kLed, LOW);
}

bool isOn() { return g_led_state; }

void setStateReporter(StateReporter reporter) { g_state_reporter = reporter; }

void setState(Source source, bool on) {
  const unsigned long now = millis();
  if (now - g_last_toggle_ms < Timings::kLedCooldownMs) {
    Serial.println("[LED] Cooldown active, ignoring request");
    return;
  }

  const bool changed = (g_led_state != on);
  g_led_state = on;
  g_last_toggle_ms = now;

  digitalWrite(Pins::kLed, on ? HIGH : LOW);
  Serial.printf("[LED] %s by %s\n", on ? "ON" : "OFF", sourceToText(source));

  if (changed && source != Source::kSinric && g_state_reporter != nullptr) {
    g_state_reporter(g_led_state);
  }
}

void blinkConfirm(unsigned long duration_ms) {
  const bool previous_state = g_led_state;
  digitalWrite(Pins::kLed, HIGH);
  delay(duration_ms);
  digitalWrite(Pins::kLed, previous_state ? HIGH : LOW);
}

}  // namespace LightController
