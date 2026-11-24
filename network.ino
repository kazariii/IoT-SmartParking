// Koneksi WiFi + MQTT (EMQX Serverless, TLS 8883).
// Berjalan di task terpisah pada core 0, sehingga handshake TLS dan
// reconnect tidak pernah menahan loop() sensor dan plang di core 1.

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "secrets.h"

#define TOPIC_PREFIX "smartparking/kelompok1/"  // samakan dengan Node-RED
#define TOPIC_SLOT     TOPIC_PREFIX "slot"
#define TOPIC_MODE     TOPIC_PREFIX "mode"
#define TOPIC_MODE_SET TOPIC_PREFIX "mode/set"
#define TOPIC_GATE_CMD TOPIC_PREFIX "gate/cmd"

const unsigned long MQTT_RETRY_MS = 5000;

WiFiClientSecure tlsClient;
PubSubClient mqtt(tlsClient);

bool payloadEquals(const byte *payload, unsigned int length, const char *text) {
  return length == strlen(text) && memcmp(payload, text, length) == 0;
}

void onMqttMessage(char *topic, byte *payload, unsigned int length) {
  if (strcmp(topic, TOPIC_MODE_SET) == 0) {
    if (payloadEquals(payload, length, "MANUAL")) {
      manualMode = true;
    } else if (payloadEquals(payload, length, "AUTO")) {
      manualMode = false;
    } else {
      Serial.printf("MQTT: payload mode tidak dikenal (%u byte)\n", length);
      return;
    }
    Serial.printf("Mode: %s\n", manualMode ? "MANUAL" : "AUTO");
  } else if (strcmp(topic, TOPIC_GATE_CMD) == 0) {
    // Hanya diteruskan; loop() yang memeriksa mode dan pengaman beam.
    if (payloadEquals(payload, length, "BUKA")) {
      pendingGateCmd = GATE_CMD_OPEN;
    } else if (payloadEquals(payload, length, "TUTUP")) {
      pendingGateCmd = GATE_CMD_CLOSE;
    } else {
      Serial.printf("MQTT: payload gate/cmd tidak dikenal (%u byte)\n", length);
    }
  }
}

// Kirim status jika berbeda dari yang terakhir terkirim. Retained, agar
// dashboard langsung tahu status saat Node-RED baru terhubung.
void publishIfChanged(const char *topic, bool value, const char *onText,
                      const char *offText, int &lastPublished) {
  if (value != lastPublished && mqtt.publish(topic, value ? onText : offText, true)) {
    lastPublished = value;
  }
}

void networkTask(void *) {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  String clientId = "smartparking-" + WiFi.macAddress();
  clientId.replace(":", "");

  tlsClient.setCACert(MQTT_CA_CERT);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqttMessage);

  unsigned long lastAttempt = 0 - MQTT_RETRY_MS;  // percobaan pertama langsung
  int lastPublishedSlot = -1;                     // -1 = belum terkirim sejak connect
  int lastPublishedMode = -1;
  bool wasOnline = false;

  for (;;) {
    bool wifiUp = WiFi.status() == WL_CONNECTED;

    // WiFi putus: tutup sesi MQTT lama agar reconnect mulai bersih saat WiFi kembali.
    if (!wifiUp && mqtt.connected()) mqtt.disconnect();

    // Fallback offline: mode Manual dibatalkan agar plang tidak terkunci
    // tanpa ada yang bisa mengontrolnya. Sensor dan plang tetap jalan di loop().
    bool online = wifiUp && mqtt.connected();
    if (wasOnline && !online) {
      manualMode = false;
      Serial.println("Offline: kembali ke mode Auto, mencoba menyambung ulang");
    }
    wasOnline = online;

    if (wifiUp && !mqtt.connected() &&
        millis() - lastAttempt >= MQTT_RETRY_MS) {
      lastAttempt = millis();
      if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
        Serial.println("MQTT: tersambung");
        mqtt.subscribe(TOPIC_MODE_SET, 1);
        mqtt.subscribe(TOPIC_GATE_CMD, 1);
        lastPublishedSlot = -1;  // kirim ulang status setelah (re)connect,
        lastPublishedMode = -1;  // agar dashboard sinkron lagi
      } else {
        Serial.printf("MQTT: gagal, state %d\n", mqtt.state());
      }
    }

    if (mqtt.connected()) {
      mqtt.loop();
      publishIfChanged(TOPIC_SLOT, sharedSlotOccupied, "TERISI", "KOSONG", lastPublishedSlot);
      publishIfChanged(TOPIC_MODE, manualMode, "MANUAL", "AUTO", lastPublishedMode);
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void startNetwork() {
  xTaskCreatePinnedToCore(networkTask, "network", 10240, nullptr, 1, nullptr, 0);
}
