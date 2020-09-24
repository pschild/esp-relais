#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <WifiHandler.h>
#include <MqttHandler.h>
#include <OTAUpdateHandler.h>

#ifndef WIFI_SSID
  #error "Missing WIFI_SSID"
#endif

#ifndef WIFI_PASSWORD
  #error "Missing WIFI_PASSWORD"
#endif

#ifndef VERSION
  #error "Missing VERSION"
#endif

const String CHIP_ID = String("ESP_") + String(ESP.getChipId());

void ping();
void lightTurnOff();
void onFooBar(char* payload);
void onPirTriggered(char* payload);
void onOtaUpdate(char* payload);
void onMqttConnected();
void onMqttMessage(char* topic, char* message);

WifiHandler wifiHandler(WIFI_SSID, WIFI_PASSWORD);
MqttHandler mqttHandler("192.168.178.28", CHIP_ID);
OTAUpdateHandler updateHandler("192.168.178.28:9042", VERSION);

Ticker pingTimer(ping, 60 * 1000);
Ticker switchOffTimer(lightTurnOff, 2 * 60 * 1000); // 2 minutes

boolean turnedOn = false;

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  pinMode(4, OUTPUT); // GPIO 4 => ESP12F

  wifiHandler.connect();
  mqttHandler.setup();
  mqttHandler.setOnConnectedCallback(onMqttConnected);
  mqttHandler.setOnMessageCallback(onMqttMessage);
  pingTimer.start();

  // start OTA update immediately
  updateHandler.startUpdate();
}

void loop() {
  mqttHandler.loop();
  updateHandler.loop();
  
  pingTimer.update();
  switchOffTimer.update();
}

void lightTurnOn() {
  turnedOn = true;
  digitalWrite(4, HIGH);
}

void lightTurnOff() {
  turnedOn = false;
  digitalWrite(4, LOW);
}

void ping() {
  const String channel = String("devices/") + CHIP_ID + String("/version");
  mqttHandler.publish(channel.c_str(), VERSION);
}

void onFooBar(char* payload) {
  switchOffTimer.stop();
  if (strcmp(payload, "on") == 0) {
    lightTurnOn();
  } else if (strcmp(payload, "off") == 0) {
    lightTurnOff();
  }
}

void onPirTriggered(char* payload) {
  if (!turnedOn) {
    lightTurnOn();
    switchOffTimer.start(); 
  }
}

void onOtaUpdate(char* payload) {
  updateHandler.startUpdate();
}

void onMqttConnected() {
  mqttHandler.subscribe("ESP_7888034/movement");
  mqttHandler.subscribe("foo/+/baz");
  mqttHandler.subscribe("otaUpdate/all");
}

void onMqttMessage(char* topic, char* message) {
  if (((std::string) topic).rfind("foo/", 0) == 0) {
    onFooBar(message);
  } else if (strcmp(topic, "otaUpdate/all") == 0) {
    onOtaUpdate(message);
  } else if (strcmp(topic, "ESP_7888034/movement") == 0) {
    onPirTriggered(message);
  }
}
