#include "iot_controller.h"

#include <WiFi.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>

#include "config.h"
#include "credentials.h"
#include "light_controller.h"

namespace {
bool onPowerState(const String& device_id, bool& state) {
  (void)device_id;
  if (state == LightController::isOn()) {
    return true;
  }

  LightController::setState(Source::kSinric, state);
  return true;
}

void reportPowerState(bool state) {
  SinricProSwitch& sw = SinricPro[DEVICE_ID];
  sw.sendPowerStateEvent(state);
}
}  // namespace

namespace IotController {

void begin() {
  Serial.println("\n[WiFi] Connecting...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print('.');
  }

  Serial.printf("\n[WiFi] Connected: %s | IP: %s\n", WIFI_SSID,
                WiFi.localIP().toString().c_str());

  SinricProSwitch& sw = SinricPro[DEVICE_ID];
  sw.onPowerState(onPowerState);
  SinricPro.begin(APP_KEY, APP_SECRET);
  SinricPro.restoreDeviceStates(true);

  LightController::setStateReporter(reportPowerState);
}

void loop() { SinricPro.handle(); }

}  // namespace IotController
