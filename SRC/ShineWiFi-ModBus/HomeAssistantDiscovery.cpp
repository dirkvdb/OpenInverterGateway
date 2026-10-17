#include "HomeAssistantDiscovery.h"

#if MQTT_SUPPORTED == 1
#include <ArduinoJson.h>
#include <cstring>
#include <functional>

#include "ShineMqtt.h"

namespace {

struct Component {
  const char* platform;
  const char* legacyId;
  const char* uniqueId;
  const char* objectId;
  const char* name;
  const char* deviceClass;
  const char* unit;
  const char* stateClass;
  const char* valueTemplate;
  const char* commandPath;
};

// Keep legacy discovery topic IDs separate from entity unique/object IDs.
const Component components[] = {
    {"sensor", "solar_energy_total" INVERTER_SUFFIX,
     "solar_production_total" INVERTER_SUFFIX,
     "solar_production_total" INVERTER_SUFFIX, HA_ENERGY_TOTAL_NAME, "energy",
     "kWh", "total_increasing", "{{ value_json.PVEnergyTotal }}", nullptr},
    {"sensor", "solar_energy_today" INVERTER_SUFFIX,
     "solar_production_today" INVERTER_SUFFIX,
     "solar_production_today" INVERTER_SUFFIX, HA_ENERGY_TODAY_NAME, "energy",
     "kWh", "total_increasing", "{{ value_json.PVEnergyToday }}", nullptr},
    {"sensor", "solar_outputpower" INVERTER_SUFFIX,
     "solar_output_power" INVERTER_SUFFIX,
     "solar_output_power" INVERTER_SUFFIX, HA_OUTPUT_POWER_NAME, "power", "W",
     "measurement", "{{ value_json.PVTotalPower }}", nullptr},
#if GROWATT_MODBUS_VERSION == 120 || GROWATT_MODBUS_VERSION == 3000
    {"sensor", "inverter_output_power" INVERTER_SUFFIX,
     "inverter_output_power" INVERTER_SUFFIX,
     "inverter_output_power" INVERTER_SUFFIX, HA_OUTPUT_POWER_INVERTER_NAME,
     "power", "W", "measurement",
#if GROWATT_MODBUS_VERSION == 120
     "{{ value_json.PVTotalPower }}",
#else
     "{{ value_json.OutputPower }}",
#endif
     nullptr},
#endif
    {"sensor", "pv1_input_power" INVERTER_SUFFIX,
     "pv1_input_power" INVERTER_SUFFIX, "pv1_input_power" INVERTER_SUFFIX,
     HA_PV1_INPUT_POWER_NAME, "power", "W", "measurement",
     "{{ value_json.PV1InputPower }}", nullptr},
    {"sensor", "pv2_input_power" INVERTER_SUFFIX,
     "pv2_input_power" INVERTER_SUFFIX, "pv2_input_power" INVERTER_SUFFIX,
     HA_PV2_INPUT_POWER_NAME, "power", "W", "measurement",
     "{{ value_json.PV2InputPower }}", nullptr},
    {"sensor", "pv1_energy_total" INVERTER_SUFFIX,
     "pv1_energy_total" INVERTER_SUFFIX, "pv1_energy_total" INVERTER_SUFFIX,
     HA_PV1_ENERGY_TOTAL_NAME, "energy", "kWh", "total_increasing",
     "{{ value_json.PV1EnergyTotal }}", nullptr},
    {"sensor", "pv2_energy_total" INVERTER_SUFFIX,
     "pv2_energy_total" INVERTER_SUFFIX, "pv2_energy_total" INVERTER_SUFFIX,
     HA_PV2_ENERGY_TOTAL_NAME, "energy", "kWh", "total_increasing",
     "{{ value_json.PV2EnergyTotal }}", nullptr},
#ifdef BATTERY_ATTACHED
    {"number", "active_power_rate" INVERTER_SUFFIX,
     "active_power_rate" INVERTER_SUFFIX, nullptr, "Active power rate", nullptr,
     "%", nullptr, "{{ value_json.ActivePowerRate }}",
     "/command/power/set/activerate"},
    {"number", "battery_discharge_power_rate" INVERTER_SUFFIX,
     "battery_discharge_power_rate" INVERTER_SUFFIX, nullptr,
     "Battery discharge power rate", nullptr, "%", nullptr,
     "{{ value_json.BDCDischargePowerRate }}",
     "/command/bdc/set/dischargepowerrate"},
    {"number", "battery_charge_power_rate" INVERTER_SUFFIX,
     "battery_charge_power_rate" INVERTER_SUFFIX, nullptr,
     "Battery charge power rate", nullptr, "%", nullptr,
     "{{ value_json.BDCChargePowerRate }}", "/command/bdc/set/chargepowerrate"},
    {"switch", "battery_ac_charging" INVERTER_SUFFIX,
     "battery_ac_charging" INVERTER_SUFFIX, nullptr, "Battery AC charging",
     nullptr, nullptr, nullptr, "{{ value_json.BDCChargeACEnabled }}",
     "/command/bdc/set/acchargeenabled"},
    {"select", "priority" INVERTER_SUFFIX, "priority" INVERTER_SUFFIX, nullptr,
     "Priority", nullptr, nullptr, nullptr,
     "{{ ['Load first', 'Battery first', 'Grid first'][value_json.Priority | int] }}",
     "/command/priority/set"},
    {"sensor", "battery_soc" INVERTER_SUFFIX, "battery_soc", "battery_soc",
     HA_BATTERY_SOC, "battery", "%", "measurement",
     "{{ value_json.BDCStateOfCharge }}", nullptr},
    {"sensor", "battery_soh" INVERTER_SUFFIX, "battery_soh", "battery_soh",
     HA_BATTERY_SOH, nullptr, "%", "measurement",
     "{{ value_json.BMSStateOfHealth }}", nullptr},
    {"sensor", "battery_charge_power" INVERTER_SUFFIX, "battery_charge_power",
     "battery_charge_power", HA_BATTERY_CHARGE_POWER, "power", "W",
     "measurement", "{{ value_json.BDCChargePower }}", nullptr},
    {"sensor", "battery_discharge_power" INVERTER_SUFFIX,
     "battery_discharge_power", "battery_discharge_power",
     HA_BATTERY_DISCHARGE_POWER, "power", "W", "measurement",
     "{{ value_json.BDCDischargePower }}", nullptr},
    {"sensor", "battery_charge_energy_today" INVERTER_SUFFIX,
     "battery_charge_energy_today", "battery_charge_energy_today",
     HA_BATTERY_CHARGE_ENERGY_TODAY, "energy", "kWh", "total_increasing",
     "{{ value_json.ChargeEnergyToday }}", nullptr},
    {"sensor", "battery_discharge_energy_today" INVERTER_SUFFIX,
     "battery_discharge_energy_today", "battery_discharge_energy_today",
     HA_BATTERY_DISCHARGE_ENERGY_TODAY, "energy", "kWh", "total_increasing",
     "{{ value_json.DischargeEnergyToday }}", nullptr},
    {"sensor", "battery_charge_energy_total" INVERTER_SUFFIX,
     "battery_charge_energy_total", "battery_charge_energy_total",
     HA_BATTERY_CHARGE_ENERGY_TOTAL, "energy", "kWh", "total_increasing",
     "{{ value_json.BDCChargeEnergyTotal }}", nullptr},
    {"sensor", "battery_discharge_energy_total" INVERTER_SUFFIX,
     "battery_discharge_energy_total", "battery_discharge_energy_total",
     HA_BATTERY_DISCHARGE_ENERGY_TOTAL, "energy", "kWh", "total_increasing",
     "{{ value_json.BDCDischargeEnergyTotal }}", nullptr},
    {"sensor", "battery_state" INVERTER_SUFFIX, "battery_state", "battery_state",
     HA_BATTERY_STATE, nullptr, nullptr, "measurement",
     "{{ value_json.BDCSysState }}", nullptr},
    {"sensor", "battery_mode" INVERTER_SUFFIX, "battery_mode", "battery_mode",
     HA_BATTERY_MODE, nullptr, nullptr, "measurement",
     "{{ value_json.BDCSysMode }}", nullptr},
#endif
};

bool writeText(Print& output, const char* text) {
  const size_t length = strlen(text);
  return output.write(reinterpret_cast<const uint8_t*>(text), length) == length &&
         output.getWriteError() == 0;
}

bool writeJson(Print& output, const JsonDocument& doc) {
  if (doc.overflowed()) return false;
  const size_t length = measureJson(doc);
  return serializeJson(doc, output) == length && output.getWriteError() == 0;
}

bool fillComponent(JsonDocument& doc, const Component& component,
                   const String& stateTopic) {
  doc.clear();
  doc["p"] = component.platform;
  doc["name"] = component.name;
  doc["uniq_id"] = component.uniqueId;
  doc["val_tpl"] = component.valueTemplate;
  if (component.objectId) {
    String entityId;
    if (!entityId.reserve(strlen(component.platform) + 1 +
                          strlen(component.objectId)) ||
        !entityId.concat(component.platform) || !entityId.concat('.') ||
        !entityId.concat(component.objectId)) {
      return false;
    }
    doc["def_ent_id"] = entityId;
  }
  if (component.deviceClass) doc["dev_cla"] = component.deviceClass;
  if (component.unit) doc["unit_of_meas"] = component.unit;
  if (component.stateClass) doc["stat_cla"] = component.stateClass;
  if (component.commandPath) {
    String commandTopic;
    if (!commandTopic.reserve(stateTopic.length() + strlen(component.commandPath)) ||
        !commandTopic.concat(stateTopic) ||
        !commandTopic.concat(component.commandPath)) {
      return false;
    }
    // Resolve the legacy ~/command/... against the actual configured state topic.
    doc["cmd_t"] = commandTopic;
  }
  if (strcmp(component.platform, "number") == 0) {
    doc["min"] = 0;
    doc["max"] = 100;
    doc["step"] = 1;
    doc["mode"] = "slider";
    doc["cmd_tpl"] = "{\"value\": {{ value | int }}}";
  } else if (strcmp(component.platform, "switch") == 0) {
    doc["stat_on"] = "1";
    doc["stat_off"] = "0";
    doc["pl_on"] = "{\"value\": 1}";
    doc["pl_off"] = "{\"value\": 0}";
  } else if (strcmp(component.platform, "select") == 0) {
    JsonArray options = doc.createNestedArray("options");
    options.add("Load first");
    options.add("Battery first");
    options.add("Grid first");
    doc["cmd_tpl"] = "{\"mode\": {{ {'L': 0, 'B': 1, 'G': 2}[value[0]] }}}";
  }
  return !doc.overflowed();
}

bool writeDeviceDiscovery(Print& output, const String& deviceId,
                          const String& deviceName, const String& stateTopic) {
  String availabilityTopic;
  if (!availabilityTopic.reserve(stateTopic.length() + strlen("/state")) ||
      !availabilityTopic.concat(stateTopic) ||
      !availabilityTopic.concat("/state")) {
    return false;
  }

  // Reused for each fragment: never allocate the complete discovery document.
  DynamicJsonDocument doc(1536);
  doc["ids"] = deviceId.c_str();
  doc["name"] = deviceName.c_str();
  doc["mf"] = "Growatt";
  if (!writeText(output, "{\"dev\":") || !writeJson(output, doc)) return false;

  doc.clear();
  doc["name"] = "OpenInverterGateway";
  doc["url"] = "https://github.com/OpenInverterGateway/OpenInverterGateway";
  if (!writeText(output, ",\"o\":") || !writeJson(output, doc)) return false;

  doc.clear();
  doc.set(stateTopic.c_str());
  if (!writeText(output, ",\"stat_t\":") || !writeJson(output, doc)) return false;

  doc.clear();
  doc.set(availabilityTopic.c_str());
  if (!writeText(output, ",\"avty_t\":") || !writeJson(output, doc) ||
      !writeText(output, ",\"pl_avail\":\"online\",\"pl_not_avail\":\"offline\",\"cmps\":{")) {
    return false;
  }

  bool first = true;
  for (const Component& component : components) {
    if (!first && !writeText(output, ",")) return false;
    first = false;
    doc.clear();
    doc.set(component.uniqueId);
    if (!writeJson(output, doc) || !writeText(output, ":") ||
        !fillComponent(doc, component, stateTopic) || !writeJson(output, doc)) {
      return false;
    }
  }
  return writeText(output, "}}");
}

bool publishLegacyTopics(ShineMqtt& mqtt, bool migrate) {
  const std::function<bool(Print&)> writer = [migrate](Print& output) {
    return writeText(output, migrate ? "{\"migrate_discovery\":true}" : "");
  };
  bool ok = true;
  for (const Component& component : components) {
    String topic;
    if (!topic.reserve(strlen("homeassistant//config") +
                       strlen(component.platform) + 1 +
                       strlen(component.legacyId)) ||
        !topic.concat("homeassistant/") || !topic.concat(component.platform) ||
        !topic.concat('/') || !topic.concat(component.legacyId) ||
        !topic.concat("/config")) {
      return false;
    }
    // Try every cleanup even if an earlier retained-topic deletion failed.
    ok = mqtt.mqttPublish(writer, topic.c_str()) && ok;
  }
  return ok;
}

}  // namespace

boolean publishHomeAssistantDiscovery(ShineMqtt& mqtt, const String& deviceName,
                                      const String& stateTopic,
                                      const String& deviceId) {
  if (deviceId.isEmpty()) return false;
  for (size_t i = 0; i < deviceId.length(); ++i) {
    const char c = deviceId[i];
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '_' || c == '-')) {
      return false;
    }
  }

  String topic;
  if (!topic.reserve(strlen("homeassistant/device//config") + deviceId.length()) ||
      !topic.concat("homeassistant/device/") || !topic.concat(deviceId) ||
      !topic.concat("/config")) {
    return false;
  }

  // HA can retain registry identity during migration, but the old entities had
  // no device metadata, so association with the new device is best effort.
  if (!publishLegacyTopics(mqtt, true)) return false;
  const std::function<bool(Print&)> writer = [&](Print& output) {
    return writeDeviceDiscovery(output, deviceId, deviceName, stateTopic);
  };
  if (!mqtt.mqttPublish(writer, topic.c_str())) return false;

  // Never delete legacy retained discovery until the device publish succeeds.
  return publishLegacyTopics(mqtt, false);
}
#endif
