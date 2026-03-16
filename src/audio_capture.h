#pragma once

#include <Arduino.h>

namespace AudioCapture {
using PollFn = void (*)();

bool begin(uint32_t sample_count);
bool recordWindow(PollFn poll_fn);
int getSignalData(size_t offset, size_t length, float* out_ptr);
void end();
}  // namespace AudioCapture
