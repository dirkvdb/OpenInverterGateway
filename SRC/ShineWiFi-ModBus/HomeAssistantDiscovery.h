#pragma once

#include "Config.h"

#if MQTT_SUPPORTED == 1
#include <Arduino.h>

class ShineMqtt;

boolean publishHomeAssistantDiscovery(ShineMqtt& mqtt, const String& deviceName,
                                      const String& stateTopic,
                                      const String& deviceId);
#endif
