#include "WifiSetup.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>

void wifiSetupBegin(const char* apName, const char* mdnsHostname) {
  WiFiManager wm;
  wm.setHttpPort(8080);  // Keep port 80 free for the REST API

  // Auto-connect using saved credentials.
  // If none exist, starts an AP with the given name and
  // blocks until the user configures credentials via the portal.
  if (!wm.autoConnect(apName)) {
    Serial.println("WiFi config portal timed out — restarting");
    ESP.restart();
  }

  Serial.print("WiFi connected, IP: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin(mdnsHostname)) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("mDNS: http://%s.local\n", mdnsHostname);
  } else {
    Serial.println("mDNS failed to start");
  }
}
