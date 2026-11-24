// Salin file ini menjadi secrets.h lalu isi. secrets.h tidak di-commit.
#pragma once

#define WIFI_SSID     "nama-wifi"
#define WIFI_PASSWORD "password-wifi"

// Dari konsol EMQX Serverless: Overview -> MQTT Connection Information.
#define MQTT_HOST     "xxxxxxxx.ala.asia-southeast1.emqxsl.com"
#define MQTT_PORT     8883
// Dari konsol EMQX Serverless: Access Control -> Authentication.
#define MQTT_USER     "esp32"
#define MQTT_PASSWORD "password-mqtt"

// Dari konsol EMQX Serverless: Overview -> CA Certificate (download, tempel isinya).
static const char MQTT_CA_CERT[] = R"EOF(
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
)EOF";
