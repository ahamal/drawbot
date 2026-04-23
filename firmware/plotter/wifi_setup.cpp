#include "wifi_setup.h"
#include "config.h"
#include <WiFi.h>
#include <ESPmDNS.h>

#if __has_include("secrets.h")
  #include "secrets.h"
#endif

#if defined(WIFI_SSID) && defined(WIFI_PASSWORD)
  #define USE_HARDCODED_WIFI 1
#else
  #include <WiFiManager.h>
#endif

void wifi_setup_init() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);

#ifdef USE_HARDCODED_WIFI
  // Local development path: credentials baked in via secrets.h.
  Serial.printf("Connecting to %s...\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > 30000) {
      Serial.println("WiFi connect timed out — restarting");
      delay(1000);
      ESP.restart();
    }
    delay(250);
    Serial.print('.');
  }
  Serial.println();
#else
  // Tutorial path: no creds in repo. ESP32 hosts an open AP "TinyRoboCo-Setup"
  // with a captive portal on first boot for the user to enter their network.
  // Saved to NVS for subsequent boots.
  WiFiManager wm;
  wm.setConfigPortalTimeout(SETUP_AP_TIMEOUT_S);
  if (!wm.autoConnect(SETUP_AP_NAME)) {
    Serial.println("WiFi setup timed out — restarting");
    delay(1000);
    ESP.restart();
  }
#endif

  if (!MDNS.begin(HOSTNAME)) {
    Serial.println("mDNS start failed");
  } else {
    MDNS.addService("http", "tcp", 80);
  }

  Serial.print("WiFi connected, IP: "); Serial.println(WiFi.localIP());
  Serial.printf("Reach the plotter at http://%s.local/status\n", HOSTNAME);
}

bool   wifi_connected() { return WiFi.status() == WL_CONNECTED; }
String wifi_ip()        { return WiFi.localIP().toString(); }
