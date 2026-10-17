#include "ShineMqtt.h"
#include "Growatt.h"
#include "GrowattTypes.h"
#if MQTT_SUPPORTED == 1
#include <TLog.h>
#include <StreamUtils.h>
#include "PubSubClient.h"
#if defined(ESP8266) && defined(MMU_IRAM_HEAP)
#include <umm_malloc/umm_heap_select.h>
#endif
#if MQTTS_ENABLED == 1 && defined(MQTTS_BROKER_CA_CERT)
#include <time.h>
#endif

namespace {

class CountingPrint : public Print {
 public:
  explicit CountingPrint(Print* destination = nullptr) : destination(destination) {}

  size_t write(uint8_t value) override { return write(&value, 1); }

  size_t write(const uint8_t* data, size_t length) override {
    if (getWriteError()) return 0;
    const size_t written = destination ? destination->write(data, length) : length;
    count += written;
    if (written != length) setWriteError();
    return written;
  }

  size_t count = 0;

 private:
  Print* destination;
};

}  // namespace

ShineMqtt::ShineMqtt(Client& wc, Growatt& inverter)
    : wifiClient(wc), mqttclient(wifiClient), inverter(inverter) {}

#if MQTTS_ENABLED == 1
ShineMqtt::ShineMqtt(WiFiClientSecure& wc, Growatt& inverter)
    : ShineMqtt(static_cast<Client&>(wc), inverter) {
  this->secureClient = &wc;
}
#endif

void ShineMqtt::mqttSetup(const MqttConfig& config) {
  this->mqttconfig = config;
  this->lastAvailability = Availability::Unknown;

  this->mqttPort = this->mqttconfig.port.toInt();
  if (this->mqttPort == 0) this->mqttPort = DefaultPort;

  logConnectionSettings();

  this->mqttclient.setServer(this->mqttconfig.server.c_str(), this->mqttPort);
  this->mqttclient.setBufferSize(BUFFER_SIZE);
  this->mqttclient.setCallback(
      [this](char* topic, byte* payload, unsigned int length) {
        this->onMqttMessage(topic, payload, length);
      });
}

void ShineMqtt::logConnectionSettings() const {
#if MQTTS_ENABLED == 1
#ifdef MQTTS_BROKER_CA_CERT
  Log.println(F("MQTT transport: TLS (CA certificate verification enabled)"));
#else
  Log.println(F("WARNING: MQTT transport: TLS without certificate verification (insecure)"));
#endif
#else
  Log.println(F("MQTT transport: plain TCP"));
#endif
  Log.print(F("MqttServer: "));
  Log.println(this->mqttconfig.server);
  Log.print(F("MqttPort: "));
  Log.println(this->mqttPort);
  Log.print(F("MqttUser: "));
  Log.println(this->mqttconfig.user);
  Log.print(F("MqttClientId: "));
  Log.println(getId());
  Log.print(F("MqttTopic: "));
  Log.println(this->mqttconfig.topic);
#if defined(ESP8266) && MQTTS_ENABLED == 1
#ifdef MMU_IRAM_HEAP
  uint32_t freeIramHeap;
  uint32_t maxFreeIramBlock;
  {
    HeapSelectIram heap;
    freeIramHeap = ESP.getFreeHeap();
    maxFreeIramBlock = ESP.getMaxFreeBlockSize();
  }
  Log.print(F("MqttIramFreeHeap: "));
  Log.println(freeIramHeap);
  Log.print(F("MqttIramMaxFreeBlock: "));
  Log.println(maxFreeIramBlock);
#else
  Log.println(F("MqttIramHeap: disabled (TLS buffers must fit in DRAM)"));
#endif
#endif
}

void ShineMqtt::logTransportError() {
#if MQTTS_ENABLED == 1
  if (this->secureClient && this->mqttclient.state() == MQTT_CONNECT_FAILED) {
    char message[160] = {};
#ifdef ESP8266
    const int error = this->secureClient->getLastSSLError(message, sizeof(message));
#else
    const int error = this->secureClient->lastError(message, sizeof(message));
#endif
    Log.print(F("MqttTlsError: "));
    Log.print(error);
    Log.print(F(" - "));
    Log.println(error ? message : "No TLS error reported (DNS/TCP failure is possible)");
  }
#endif
  Log.print(F("MqttFreeHeapAfterFailure: "));
  Log.println(ESP.getFreeHeap());
  Log.print(F("MqttLocalIP: "));
  Log.println(WiFi.localIP().toString());
  Log.print(F("MqttGateway: "));
  Log.println(WiFi.gatewayIP().toString());
  Log.print(F("MqttDnsServer: "));
  Log.println(WiFi.dnsIP().toString());
  Log.print(F("MqttRSSI: "));
  Log.println(WiFi.RSSI());

  // Keep the hostname for the actual connection (TLS SNI and hostname checks).
  IPAddress address;
  if (WiFi.hostByName(this->mqttconfig.server.c_str(), address) == 1) {
    Log.print(F("MqttDnsLookupAfterFailure: "));
    Log.println(address.toString());
  } else {
    Log.println(F("MqttDnsLookupAfterFailure: failed"));
  }
}

String ShineMqtt::getId() {
#ifdef ESP8266
  uint64_t id = ESP.getChipId();
#elif ESP32
  uint64_t id = ESP.getEfuseMac();
#endif
  return WiFi.getHostname() + String(id & 0xffffffff);
}

boolean ShineMqtt::mqttEnabled() { return !this->mqttconfig.server.isEmpty(); }
boolean ShineMqtt::mqttConnected() { return this->mqttclient.connected(); }

// -------------------------------------------------------
// Check the Mqtt status and reconnect if necessary
// -------------------------------------------------------
bool ShineMqtt::mqttReconnect() {
  if (!this->mqttEnabled()) {
    // No server configured
    return false;
  }

  if (WiFi.status() != WL_CONNECTED) return false;

  if (this->mqttclient.connected()) return true;

  if (millis() - this->previousConnectTryMillis >= (5000)) {
#if MQTTS_ENABLED == 1 && defined(MQTTS_BROKER_CA_CERT)
    // A freshly booted device has no usable clock for certificate validity checks.
    if (time(nullptr) < 1609459200) {  // 2021-01-01 UTC
      Log.println(F("MQTTS: waiting for NTP time synchronization"));
      this->previousConnectTryMillis = millis();
      return false;
    }
#endif
    Log.print(F("Attempting MQTT connection..."));

    // Run only once every 5 seconds
    this->previousConnectTryMillis = millis();
    // Availability is separate from telemetry so the will never replaces sensor data.
    const String availabilityTopic = this->mqttconfig.topic + "/state";
    const uint32_t freeHeapBeforeConnect = ESP.getFreeHeap();
#ifdef ESP8266
    const uint32_t maxFreeBlockBeforeConnect = ESP.getMaxFreeBlockSize();
#endif
    if (this->mqttclient.connect(getId().c_str(), this->mqttconfig.user.c_str(),
                                 this->mqttconfig.pwd.c_str(),
                                 availabilityTopic.c_str(), 1, 1, "offline")) {
      Log.println(F("connected"));

      // The broker may have sent the will while our cached state was still online.
      this->lastAvailability = Availability::Unknown;
      // Do not replay retained online availability before a fresh inverter read.
      if (!mqttPublishAvailability(false)) {
        this->wifiClient.stop();
        return false;
      }

      String commandTopic = this->mqttconfig.topic + "/command/#";
      if (this->mqttclient.subscribe(commandTopic.c_str(), 1)) {
        Log.println("Subscribed to " + commandTopic);
      } else {
        Log.println("Failed to subscribe to " + commandTopic);
      }
      return true;
    } else {
      Log.print(F("failed, rc="));
      Log.print(this->mqttclient.state());
      Log.println(F(" try again in 5 seconds"));
      Log.println(F("MQTT Connect failed"));
      Log.print(F("MqttFreeHeapBeforeConnect: "));
      Log.println(freeHeapBeforeConnect);
#ifdef ESP8266
      Log.print(F("MqttMaxFreeBlockBeforeConnect: "));
      Log.println(maxFreeBlockBeforeConnect);
#endif
      logTransportError();
      logConnectionSettings();
      previousConnectTryMillis = millis();
    }
  }
  return false;
}

boolean ShineMqtt::mqttPublishAvailability(bool online) {
  if (!this->mqttclient.connected()) {
    this->lastAvailability = Availability::Unknown;
    return false;
  }
  const Availability next = online ? Availability::Online : Availability::Offline;
  if (this->lastAvailability == next) return true;

  const String topic = this->mqttconfig.topic + "/state";
  const std::function<bool(Print&)> writer = [online](Print& output) {
    const char* payload = online ? "online" : "offline";
    return output.print(payload) == strlen(payload) && !output.getWriteError();
  };
  if (!mqttPublish(writer, topic.c_str())) return false;
  this->lastAvailability = next;
  return true;
}

boolean ShineMqtt::mqttPublish(const String& jsonString, const char* topic) {
  if (!topic) {
    topic = this->mqttconfig.topic.c_str();
  }

  const size_t packetSize =
      MQTT_MAX_HEADER_SIZE + 2 + strlen(topic) + jsonString.length();
  if (packetSize > BUFFER_SIZE) {
    Log.println(F("MQTT message too long for buffer"));

    return false;
  }

  Log.print(F("publish MQTT message... "));
  if (this->mqttclient.connected()) {
    bool res = this->mqttclient.publish(topic, jsonString.c_str(), true);
    Log.println(res ? "succeed" : "failed");

    return res;
  } else {
    Log.println(F("not connected"));

    return false;
  }
}

boolean ShineMqtt::mqttPublish(JsonDocument& doc, String topic) {
  Log.print(F("publish MQTT json message... "));

  if (topic.isEmpty()) {
    topic = this->mqttconfig.topic;
  }

  if (doc.overflowed()) {
    Log.println(F("JSON document overflowed; MQTT publish skipped"));
    return false;
  }
  const std::function<bool(Print&)> writer = [&](Print& output) {
    return serializeJson(doc, output) == measureJson(doc) && !output.getWriteError();
  };
  const bool res = mqttPublish(writer, topic.c_str());
  Log.println(res ? "succeed" : "failed");
  return res;
}

boolean ShineMqtt::mqttPublish(const std::function<bool(Print&)>& writer,
                               const char* topic) {
  if (!this->mqttclient.connected()) return false;

  CountingPrint measurement;
  if (!writer(measurement) || measurement.getWriteError()) {
    Log.println(F("Unable to measure MQTT payload"));
    return false;
  }
  if (!this->mqttclient.beginPublish(topic, measurement.count, true)) {
    this->wifiClient.stop();
    Log.println(F("Unable to start MQTT publish; connection closed"));
    return false;
  }

  CountingPrint output(&this->mqttclient);
  BufferingPrint bufferedOutput(output, BUFFER_SIZE);
  const bool generated = writer(bufferedOutput);
  bufferedOutput.flush();
  const bool complete = generated && !output.getWriteError() &&
                        output.count == measurement.count;
  if (!complete) {
    // A partial MQTT packet must not be followed by another packet on this socket.
    this->wifiClient.stop();
    Log.println(F("Incomplete MQTT payload; connection closed"));
    return false;
  }
  return this->mqttclient.endPublish();
}

void ShineMqtt::onMqttMessage(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<1024> req;
  StaticJsonDocument<1024> res;
  String strTopic(topic);

  Log.print(F("MQTT message arrived ["));
  Log.print(strTopic);
  Log.print(F("] "));

  String command =
      strTopic.substring(String(this->mqttconfig.topic + "/command/").length());
  if (command.isEmpty()) {
    return;
  }

  this->inverter.HandleCommand(command, payload, length, req, res);
  mqttPublish(res, this->mqttconfig.topic + "/result");
}

void ShineMqtt::loop() { this->mqttclient.loop(); }
#endif
