#include "src/button_controller.h"
#include "src/inference_controller.h"
#include "src/iot_controller.h"
#include "src/light_controller.h"

namespace {
void pollControllers() {
  IotController::loop();
  ButtonController::loop();
}
}  // namespace

void setup() {
  Serial.begin(115200);

  LightController::begin();
  ButtonController::begin();
  IotController::begin();
  InferenceController::begin();
}

void loop() {
  pollControllers();
  InferenceController::loop(pollControllers);
}
