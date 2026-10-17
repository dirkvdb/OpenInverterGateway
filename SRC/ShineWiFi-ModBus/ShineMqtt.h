#pragma once

#include "Config.h"

#if MQTT_SUPPORTED == 1
#include <Arduino.h>
#include "ShineWifi.h"
#include "Growatt.h"
#include <PubSubClient.h>
#include <stdbool.h>
#include <functional>

typedef struct {
  String server;
  String port;
  String topic;
  String user;
  String pwd;
} MqttConfig;

class ShineMqtt {
 public:
  ShineMqtt(WiFiClient& wc, Growatt& inverter);
  void mqttSetup(const MqttConfig& config);
  bool mqttReconnect();
  boolean mqttPublishAvailability(bool online);
  boolean mqttPublish(const String& JsonString, const char* topic = nullptr);
  boolean mqttPublish(JsonDocument& doc, String topic = "");
  // The writer runs twice: once to measure, then to stream a retained payload.
  boolean mqttPublish(const std::function<bool(Print&)>& writer, const char* topic);
  boolean mqttEnabled();
  boolean mqttConnected();
  void onMqttMessage(char* topic, byte* payload, unsigned int length);
  void loop();

 private:
  enum class Availability { Unknown, Offline, Online };
  Availability lastAvailability = Availability::Unknown;
  WiFiClient& wifiClient;
  unsigned long previousConnectTryMillis = 0;
  MqttConfig mqttconfig;
  PubSubClient mqttclient;
  Growatt& inverter;
  static String getId();
};
#endif
