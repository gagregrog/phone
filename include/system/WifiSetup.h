#pragma once
#include <Arduino.h>

// Connects to WiFi using saved credentials, or spawns a captive portal AP
// to collect them. Blocks during the configuration portal only.
void wifiSetupBegin(const char* apName = "PhoneSetup",
                    const char* mdnsHostname = "phone");
