#pragma once

#if __has_include("secrets.h")
#include "secrets.h"
#elif __has_include("../secrets.h")
#include "../secrets.h"
#elif __has_include("secrets.example.h")
#include "secrets.example.h"
#elif __has_include("../secrets.example.h")
#include "../secrets.example.h"
#else
#error "Missing secrets.h and secrets.example.h."
#endif

#ifndef WIFI_SSID
#error "WIFI_SSID is not defined."
#endif

#ifndef WIFI_PASS
#error "WIFI_PASS is not defined."
#endif

#ifndef APP_KEY
#error "APP_KEY is not defined."
#endif

#ifndef APP_SECRET
#error "APP_SECRET is not defined."
#endif

#ifndef DEVICE_ID
#error "DEVICE_ID is not defined."
#endif
