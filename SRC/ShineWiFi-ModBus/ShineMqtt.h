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
#if MQTTS_ENABLED == 1
  static constexpr uint16_t DefaultPort = 8883;
#else
  static constexpr uint16_t DefaultPort = 1883;
#endif

  ShineMqtt(Client& wc, Growatt& inverter);
#if MQTTS_ENABLED == 1
  ShineMqtt(WiFiClientSecure& wc, Growatt& inverter);
#endif
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
  Client& wifiClient;
#if MQTTS_ENABLED == 1
  WiFiClientSecure* secureClient = nullptr;
#endif
  unsigned long previousConnectTryMillis = 0;
  MqttConfig mqttconfig;
  uint16_t mqttPort = DefaultPort;
  PubSubClient mqttclient;
  Growatt& inverter;
  static String getId();
  void logConnectionSettings() const;
  void logTransportError();
};
#endif
