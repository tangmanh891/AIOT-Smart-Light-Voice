#pragma once

#include "audio_capture.h"

namespace InferenceController {
void begin();
void loop(AudioCapture::PollFn poll_fn);
}  // namespace InferenceController
