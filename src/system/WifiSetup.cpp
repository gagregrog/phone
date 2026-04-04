#include "system/WifiSetup.h"
#include "system/Logger.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

void wifiSetupBegin(const char* apName, const char* mdnsHostname) {
  WiFiManager wm;
  // Auto-connect using saved credentials.
  // If none exist, starts an AP with the given name and
  // blocks until the user configures credentials via the portal.
  if (!wm.autoConnect(apName)) {
    Serial.println("WiFi config portal timed out — restarting");
    ESP.restart();
  }

  logger.infof("WiFi connected, IP: %s", WiFi.localIP().toString().c_str());

  if (MDNS.begin(mdnsHostname)) {
    MDNS.addService("http", "tcp", 80);
    logger.infof("mDNS: http://%s.local", mdnsHostname);
  } else {
    logger.warn("mDNS failed to start");
  }

  ArduinoOTA.setHostname(mdnsHostname);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.begin();
  logger.info("OTA ready");
}
