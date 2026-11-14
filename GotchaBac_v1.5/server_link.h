// WiFi connection and telemetry upload to the GotchaBac server.
//
// The HTTP request runs in its own FreeRTOS task on core 0, so a slow or
// unreachable server never freezes the display or touch handling in loop().

#pragma once

#include <WiFi.h>
#include <HTTPClient.h>

#include "secrets.h"

struct TelemetrySnapshot {
  int raw;
  float voltage;
  float hunger;
  float happiness;
  float health;
  int battery;
  float cpu;
  bool dead;
  char state[16];
  unsigned long uptimeMs;
};

static QueueHandle_t telemetryQueue = nullptr;
static char serverDeviceId[24];

// True while the last upload succeeded.
static volatile bool serverOnline = false;

static void serverLinkTask(void *parameter) {

  TelemetrySnapshot snap;
  bool wifiWasConnected = false;
  int lastCode = 0;

  String url =
    String("http://") + SERVER_HOST + ":" + String(SERVER_PORT) + "/api/telemetry";

  for (;;) {

    // Sleep until loop() publishes a new snapshot
    if (xQueueReceive(telemetryQueue, &snap, portMAX_DELAY) != pdTRUE) {
      continue;
    }

    bool wifiConnected = WiFi.status() == WL_CONNECTED;

    if (wifiConnected != wifiWasConnected) {

      wifiWasConnected = wifiConnected;

      if (wifiConnected) {
        Serial.print("WiFi connected, IP: ");
        Serial.println(WiFi.localIP());
      }
      else {
        Serial.println("WiFi disconnected");
      }
    }

    if (!wifiConnected) {
      serverOnline = false;
      continue;
    }

    char body[320];

    snprintf(
      body,
      sizeof(body),
      "{\"deviceId\":\"%s\",\"raw\":%d,\"voltage\":%.3f,\"state\":\"%s\","
      "\"hunger\":%.1f,\"happiness\":%.1f,\"health\":%.1f,"
      "\"battery\":%d,\"cpu\":%.1f,\"dead\":%s,\"uptimeMs\":%lu}",
      serverDeviceId,
      snap.raw,
      snap.voltage,
      snap.state,
      snap.hunger,
      snap.happiness,
      snap.health,
      snap.battery,
      snap.cpu,
      snap.dead ? "true" : "false",
      snap.uptimeMs
    );

    WiFiClient client;
    HTTPClient http;

    http.setConnectTimeout(3000);
    http.setTimeout(3000);

    int code = -1;

    if (http.begin(client, url)) {

      http.addHeader("Content-Type", "application/json");
      http.addHeader("X-API-Key", SERVER_API_KEY);

      code = http.POST(body);

      http.end();
    }

    serverOnline = (code == 200);

    // Log only when the result changes, to keep Serial readable
    if (code != lastCode) {

      lastCode = code;

      if (code == 200) {
        Serial.println("Server upload OK");
      }
      else if (code > 0) {
        Serial.print("Server rejected upload, HTTP ");
        Serial.println(code);
      }
      else {
        Serial.print("Server unreachable: ");
        Serial.println(HTTPClient::errorToString(code));
      }
    }
  }
}

// Call once from setup(). Returns immediately; WiFi connects in the background.
void serverLinkBegin() {

  snprintf(
    serverDeviceId,
    sizeof(serverDeviceId),
    "gotcha-%012llX",
    (unsigned long long)ESP.getEfuseMac()
  );

  Serial.print("Device ID: ");
  Serial.println(serverDeviceId);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Length 1: only the newest snapshot matters
  telemetryQueue = xQueueCreate(1, sizeof(TelemetrySnapshot));

  xTaskCreatePinnedToCore(
    serverLinkTask,
    "serverLink",
    8192,
    nullptr,
    1,
    nullptr,
    0
  );
}

// Call from loop(). Never blocks; replaces any snapshot not yet sent.
void serverLinkPublish(const TelemetrySnapshot &snap) {

  if (telemetryQueue) {
    xQueueOverwrite(telemetryQueue, &snap);
  }
}
