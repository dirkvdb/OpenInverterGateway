#pragma once

#include "Config.h"

#ifndef MQTTS_ENABLED
#if MQTT_SUPPORTED == 1 && defined(MQTTS_BROKER_CA_CERT)
#define MQTTS_ENABLED 1
#else
#define MQTTS_ENABLED 0
#endif
#endif

#if MQTT_SUPPORTED == 1 && MQTTS_ENABLED == 1 && defined(MQTTS_BROKER_CA_CERT)
#ifndef DEFAULT_NTP_SERVER
#error "Verified MQTTS requires DEFAULT_NTP_SERVER for certificate validation"
#endif
#endif

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#elif ESP32
#include <WiFi.h>
#include <WebServer.h>
#endif

#if MQTTS_ENABLED == 1
#include <WiFiClientSecure.h>
#else
#include <WiFiClient.h>
#endif

#if MQTTS_ENABLED == 1
extern WiFiClientSecure espClient;
#else
extern WiFiClient espClient;
#endif

bool ShineWifiDisconnect();