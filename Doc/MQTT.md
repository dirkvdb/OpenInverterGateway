# Homeassistant configuration

To make use of MQTT protocol that sends data from Growatt datalogger you need a MQTT broker and MQTT integration with this broker in Home Assistant.

If you don't have your own MQTT broker, then just use the existing add-on in HomeAssistant. Here is how you do it.

1. Settings -> Add-ons go to Add On store and look for `Mosquitto broker`, install it. Default settings will suffice.
2. Start the Mosquitto add-on.
3. Go to Settings -> Devices & services. In Integrations it will automatically suggest `MQTT` integration, create it with default settings.

Now let's create a user for mqtt broker to use from Growatt datalogger.

1. Go to Settings -> People -> Users. Press `Add User`, username: `growatt`, password: `your password`, mark it as `local only`.

Time to configure MQTT settings on Growatt datalogger.
If it's already in your network go to its IP address and press `Start config access point` or press AP button on the datalogger
(see specific info for your logger how to enable AP mode).
Connect to `Growatt` network, go to <http://192.168.4.1>

Choose `Setup` and set the mqtt parameters:

- mqtt server ip: ip of your home assistant instance
- mqtt port: leave default (1883)
- mqtt user: growatt (Or whatever user you have created)
- mqtt password: \<password of that user>

Save the settings.

Disconnect from AP.
It should eventually turn off AP and reconnect to your home WiFi and start sending data.
You can check if it is successful going to IP address of your Growatt device and checking logs.  

## MQTT over TLS (MQTTS)

MQTTS is supported on ESP8266 (including ShineWiFi-X/S) and ESP32. TLS is selected
at compile time; setting port `8883` alone does not enable encryption.

1. Configure a TLS listener on your broker (normally port `8883`).
2. In `Config.h`, keep `MQTT_SUPPORTED` set to `1` and set `MQTTS_ENABLED` to `1`.
   Optionally define `MQTTS_BROKER_CA_CERT` as the complete PEM-encoded CA
   certificate that signed your broker's server certificate. Follow the quoted,
   newline-escaped format in `Config.h.example`, replacing all sample certificate
   lines. For a private CA, use your private CA's public certificate, **not its
   private key**. The broker must serve any intermediate certificates needed to
   build its chain. If no CA is defined, TLS uses `setInsecure()` and skips
   certificate and hostname validation.
3. When using a CA, set `DEFAULT_NTP_SERVER` to a reachable NTP server. A valid
   clock is required to check certificate expiration; MQTT waits for time
   synchronization without blocking inverter polling or the web interface.
   NTP uses UDP port `123`. Without a CA, MQTT does not wait for NTP.
4. Rebuild and flash the firmware.
5. In the configuration portal, set the MQTT server without a scheme or path
   (for example, `mqtt.example.com`, not `mqtts://mqtt.example.com`). When using
   a CA, the hostname or IP address must be covered by the broker certificate.
   Set the port to `8883` or your broker's TLS listener port. Topic, username,
   and password work as before.

TLS builds default to port `8883` when the saved port is absent or blank.
**Previously saved ports are preserved**, so change an existing `1883` setting
when switching to TLS. If `MQTTS_ENABLED` is not explicitly set, defining
`MQTTS_BROKER_CA_CERT` enables TLS for compatibility; otherwise plain MQTT is
used. Setting `MQTTS_ENABLED` to `0` explicitly selects plain MQTT with default
port `1883`, even if a CA is defined.

**Warning:** with `MQTTS_ENABLED` set to `1` but no `MQTTS_BROKER_CA_CERT`, traffic
is encrypted but the broker is not authenticated. An attacker can impersonate
it and intercept credentials, telemetry, or commands. The firmware logs an
insecure-TLS warning in this mode. Define a trusted CA to enable verification.
An invalid or untrusted certificate in verified mode still fails the connection;
there is no automatic fallback to insecure TLS or plain MQTT.

Client-certificate authentication (mutual TLS) is not configured; use broker
username/password authentication if required. Updating the trusted CA requires
rebuilding and flashing the firmware.

For troubleshooting, check the debug log for `MQTT transport: TLS` and
`MQTTS: waiting for NTP time synchronization`. If connection attempts fail after
time synchronization, check DNS, the broker port, the CA and intermediate
certificate chain, the certificate hostname and validity, and broker logs.
Connection failures log the effective host, port, transport, verification mode,
username, client ID, and topic, but never the password. They also log the TLS
library error (for transport failures), free heap before and after the attempt,
the largest free block before the attempt on ESP8266, local IP, gateway, DNS
server, Wi-Fi RSSI, and a fresh DNS lookup performed after the failure. A zero
TLS error does not rule out a transport problem: DNS or TCP may have failed
before the handshake. The diagnostic DNS lookup can add a delay to failed retries.
TLS uses additional RAM, which is particularly relevant on ESP8266 devices.
The ESP8266 PlatformIO environments enable the core's shared IRAM heap using
`PIO_FRAMEWORK_ARDUINO_MMU_CACHE16_IRAM48_SECHEAP_SHARED`. This uses a 16 KB
instruction cache instead of 32 KB, making unused space in 48 KB of instruction
RAM available as a second heap. BearSSL preferentially allocates its TLS I/O
buffers there, preserving DRAM for the application. The full 16 KB TLS receive
buffer is retained; this does not require smaller-record support from the broker.
`MqttIramFreeHeap` and `MqttIramMaxFreeBlock` report the second heap separately
from the DRAM figures. Error `-1000` means BearSSL could not allocate its TLS
structures or buffers; check both heaps when diagnosing it.

## Automatic discovery

Set `MQTT_SUPPORTED` and `HA_PUBLISH_CONFIG` to `1` in `Config.h` to enable Home Assistant MQTT discovery.
Set `HASS_DEVICE` to a stable identifier unique to this gateway (for example, `"growatt_inverter_home"`).
Use only letters, digits, underscores, and hyphens. This value is used unchanged in the discovery topic
and as the device identifier (`dev.ids`).
Set `DEVICE_NAME` to the device name shown in Home Assistant (for example, `"Growatt inverter home"`).
If `DEVICE_NAME` is not defined, the configured gateway hostname is used.

All existing sensors and, when `BATTERY_ATTACHED` is enabled, battery controls are published together
in one retained discovery message:

```text
homeassistant/device/<HASS_DEVICE>/config
```

The payload contains shared device information (`dev`), origin information (`o`), component definitions
(`cmps`), the configured MQTT telemetry topic (`stat_t`), and shared availability settings
(`avty_t`, `pl_avail`, `pl_not_avail`). Availability applies to all sensors and controls.
Each component declares its platform (`p`),
such as `sensor`, `number`, `switch`, or `select`. No extra sensors are added; the existing unique IDs,
value templates, units, and state/command topics are preserved. Keep `HASS_DEVICE` unchanged to preserve
the device identity; changing only `DEVICE_NAME` does not create a second device.
If you previously flashed firmware using the MAC-based discovery topic, clear that old retained
`homeassistant/device/openinvertergateway_<MAC>/config` message before switching to `HASS_DEVICE`.

Use a Home Assistant version that supports [MQTT device discovery](https://www.home-assistant.io/integrations/mqtt/#device-discovery-payload).
After flashing, look under **Settings → Devices & services → MQTT → Devices** for `DEVICE_NAME`.
Home Assistant may prefix entity display names with the device name.

When upgrading from per-entity discovery, the gateway sends discovery migration markers to the old
configuration topics, publishes the device configuration, and then clears the old retained discovery
messages. Keep Home Assistant connected during the first boot after upgrading so it can process the
migration. The previous discovery payloads did not include device information, so check the Home
Assistant MQTT logs and existing entity IDs after upgrading, especially if automations depend on them.

The discovery payload is streamed, so its size is not limited by the MQTT client's 512-byte buffer.

### Telemetry and availability topics

The configured main topic (for example, `energy/solar`) remains the JSON telemetry topic (`stat_t`).
Availability is published separately as a retained plain-text message on the main topic plus `/state`
(for example, `energy/solar/state`), referenced by discovery's shared `avty_t`:

- `online` (`pl_avail`): valid inverter telemetry is available.
- `offline` (`pl_not_avail`): the inverter reports status `-1`, an inverter read fails, or the gateway
  disconnects from MQTT (via the retained Last Will for an unexpected disconnect).

Availability is published only when it changes, not on every telemetry update. After each MQTT
reconnect, the gateway republishes `offline` until fresh telemetry succeeds, then publishes `online`.
Failed availability publishes are retried rather than cached as successful.

The main JSON topic no longer receives partial negative-status payloads such as
`{"InverterStatus":-1}` on failure. It retains the last valid telemetry instead; consumers should use
`/state` to determine whether those readings are currently available. Device and entity identities,
telemetry value templates, and command topics are unchanged.

## MQTT inside Homeassistant configuration

This will put the inverter on the energy dashboard. Add it to your `/config/configuration.yaml`.
(You have SSH Add-on for Home assistant installed and can SSH to it).

Different protocol versions send different packages thus might need different configuration.

To find which properties are available checkout name of the registers in relevant cpp file.

E.g. for protocol 3.05 in [Growatt305.cpp](/SRC/ShineWiFi-ModBus/Growatt305.cpp)

Here are some examples:

- [Version for protocol 1.24](#version-for-protocol-124)
- [Version for protocol 3.05](#version-for-protocol-305)

### Version for protocol 1.24

```yaml
mqtt:
  sensor:
    - state_topic: "energy/solar"
      unique_id: "growatt_wr_total_production"
      name: "Growatt.TotalGenerateEnergy"
      unit_of_measurement: "kWh"
      value_template: "{{ float(value_json.TotalGenerateEnergy) | round(1) }}"
      device_class: energy
      state_class: total_increasing
      json_attributes_topic: "energy/solar"
      payload_available: "online"
      payload_not_available: "offline"
      availability_topic: "energy/solar/state"
```

To extract the current AC Power you have to add a sensor template.

```yaml
template:
  - sensor:
      - name: "Growatt inverter AC Power"
        unit_of_measurement: "W"
        state: "{{ float(state_attr('sensor.growatt_inverter', 'OutputPower')) }}"
```

To send commands you can use the MQTT Publish service. Note that the datetime commands are currently only available on the 1.24 modbus version.

```yaml
service: mqtt.publish
data:
  qos: "1"
  topic: energy/solar/command/datetime/get
  payload_template: |
    {
      "correlationId": "ha-datetime-get"
    }
```

```yaml
service: mqtt.publish
data:
  qos: "1"
  topic: energy/solar/command/datetime/set
  payload_template: |
    {
      "correlationId": "ha-datetime-set",
      "value": "{{ now().strftime('%Y-%m-%d %H:%M:%S') }}"
    }
```

To receive responses to commands you can use templates.

```yaml
- trigger:
    platform: mqtt
    topic: energy/solar/result
    value_template: "{{ value_json.correlationId }}"
    payload: "ha-datetime-get"
  sensor:
    name: Growatt - Inverter date/time
    state: "{{ trigger.payload_json.value }}"
    attributes:
      success: "{{ trigger.payload_json.success }}"
      message: "{{ trigger.payload_json.message }}"
```

```yaml
- trigger:
    platform: mqtt
    topic: energy/solar/result
    value_template: "{{ value_json.correlationId }}"
    payload: "ha-datetime-set"
  binary_sensor:
    name: Growatt - Inverter set date/time
    state: "{{ trigger.payload_json.success }}"
    attributes:
      message: "{{ trigger.payload_json.message }}"
```

You could create an automation triggered by a binary sensor on the success JSON attribute if you want to be notified when
a command has failed for example.

The following MQTT commands are defined for protocol 1.24:

```plaintext
datetime/get
datetime/set
batteryfirst/get
batteryfirst/set/powerrate
batteryfirst/set/stopsoc
batteryfirst/set/acchargeenabled
batteryfirst/set/timeslot
gridfirst/get
gridfirst/set/powerrate
gridfirst/set/stopsoc
gridfirst/set/timeslot
power/get/activerate
power/set/activerate
```

### Version for protocol 3.05

```yaml
mqtt:
  sensor:
    - state_topic: "energy/solar"
      unique_id: "growatt_total_production"
      name: "Growatt. Energy Total"
      unit_of_measurement: "kWh"
      value_template: "{{ float(value_json.EnergyTotal) | round(1) }}"
      device_class: energy
      state_class: total_increasing
      json_attributes_topic: "energy/solar"
      payload_available: "online"
      payload_not_available: "offline"
      availability_topic: "energy/solar/state"

    - name: "Growatt Status"
      unique_id: "growatt_status"
      state_topic: "energy/solar"
      value_template: "{{ value_json.Status }}"

    - name: "Growatt DC Voltage"
      unique_id: "growatt_dc_voltage"
      unit_of_measurement: "V"
      device_class: "voltage"
      state_topic: "energy/solar"
      value_template: "{{ value_json.DcVoltage }}"
      
    - name: "Growatt AC Frequency"
      unique_id: "growatt_ac_frequency"
      unit_of_measurement: "Hz"
      state_class: "measurement"
      state_topic: "energy/solar"
      device_class: "frequency"
      value_template: "{{ value_json.AcFrequency }}"  

    - name: "Growatt AC Voltage"
      unique_id: "growatt_ac_voltage"
      unit_of_measurement: "V"
      state_class: "measurement"
      device_class: "voltage"
      state_topic: "energy/solar"
      value_template: "{{ value_json.AcVoltage }}"        
  
    - name: "Growatt AC Power"
      unique_id: "growatt_ac_power"
      unit_of_measurement: "W"
      state_class: "measurement"
      device_class: "power"
      state_topic: "energy/solar"
      value_template: "{{ value_json.AcPower }}"              
      
    - name: "Growatt Energy Today"
      unique_id: "growatt_energy_today"
      unit_of_measurement: "kWh"
      state_class: "total_increasing"
      device_class: "energy"
      state_topic: "energy/solar"
      value_template: "{{ value_json.EnergyToday }}"  
      
    - name: "Growatt Temperature"
      unique_id: "growatt_temperature"
      unit_of_measurement: "°C"
      state_class: "measurement"
      device_class: "temperature"
      state_topic: "energy/solar"
      value_template: "{{ value_json.Temperature }}"
```

## Direct Access

Using `mosquitto_sub` / `mosquitto_pub` directly from the cli.

Subscribe to MQTT events from the stick:

```shell
mosquitto_sub -h <ip> -u <mqttuser> -P <mqttpw> -t "energytest/" -v
```

Get battery first state:

```shell
mosquitto_pub -h <ip> -u <mqttuser> -P <mqttpw> -m "{\"correlationId\": \"ha-batteryfirst-get\"}" -t energytest/solar/command/batteryfirst/get
```

Example how to limit output power in percent via `mosquitto_pub` from cli:

```shell
mosquitto_pub -h <mqttip> -u <mqttuser> -P <mqttpw> \
    -t "<base-topic>/command/power/set/activeRate" -m "{ \"value\": 50 }"
```
