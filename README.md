# esphome-igrill

ESPHome Custom Component for the iGrill Bluetooth Thermometers
This component will let you use a supported ESP32 to read sensor values from IGrill bluetooth thermometers and Pulse BBQ's.

> [!IMPORTANT]
> To be compatible with [ESPHome 2026.3.0](https://esphome.io/changelog/2026.3.0) or above, you need to `v1.4` or later of this component

## Installation

To use this component, include it as an [External component](https://esphome.io/components/external_components.html)

```yaml
external_components:
  - source: github://bendikwa/esphome-igrill@v1.4
```

## Device discovery

IGrill devices can be found using the `igrill_ble_listener`

To find out your device’s MAC address, add the following to your ESPHome configuration:

```yaml
esp32_ble_tracker:
igrill_ble_listener:
```

The device will then listen for nearby devices, and display a message like this one:

```
[I][igrill_ble_listener:029]: Found IGrill device Name: iGrill_mini (MAC: 70:91:8F:XX:XX:XX)
```

Once the device is found, take note of the device MAC address. You will use it when configuring a sensor below.
You can now remove the `igrill_ble_listener` device tracker from your configuration.

## Supported Devices
In principle, all IGrill devices, including the Pulse 2000 are supported, but I do not own all of them. The ones with a checkmark in the list are confirmed working IGrill models:

- [x] IGrill mini
- [ ] IGrill mini V2
- [x] IGrill V2 - Thanks to [stogs](https://github.com/stogs) for verifying
- [X] IGrill V202
- [x] IGrill V3
- [x] Weber Pulse 1000 Thanks to [samvanh](https://github.com/samvanh) for verifying
- [x] Weber Pulse 2000 Thanks to [PaulAntonDeen](https://github.com/PaulAntonDeen) for testing and verifying
- [x] iDevices LLC Kitchen Bleutooth Smart Thermometer Thanks to [Burak](https://github.com/108burakk) for testing and verifying

If you own one of the untested models, I would be thankfull if you create a ticket so we can get it confirmed working.

### Target temperature (read/write) verified
 
The BLE write format for the alarm/target temperature characteristic was
reverse-engineered from a packet capture of the official app, and confirmed
working on the model below. Other models use the same characteristic
pattern for temperature reading, so are likely compatible, but are
unconfirmed for **writing** a target until tested:
 
- [x] IGrill V2 - confirmed read + write, verified against a BLE HCI capture
      of the official app setting an alarm on the same hardware
- [ ] IGrill mini
- [ ] IGrill mini V2
- [ ] IGrill V202
- [ ] IGrill V3
- [ ] Weber Pulse 1000
- [ ] Weber Pulse 2000 (uses a different characteristic for heating-element
      setpoints; not covered by this feature at all currently)
- [ ] iDevices LLC Kitchen Bluetooth Smart Thermometer
 
If you test this on an untested model, please comment on the PR/issue so
the list can be updated.

## Configuration example

```yaml
esp32_ble_tracker:

ble_client:
  - mac_address: 70:91:8F:XX:XX:XX
    id: igrill_device

sensor:
  - platform: igrill
    ble_client_id: igrill_device
    id: igrill_component
    update_interval: 30s # default
    battery_level:
      name: "IGrill battery"
    temperature_probe1:
      name: "IGrill temp probe 1"
    temperature_probe2:
      name: "IGrill temp probe 2"
    temperature_probe3:
      name: "IGrill temp probe 3"
    temperature_probe4:
      name: "IGrill temp probe 4"

number:
  - platform: igrill
    igrill_id: igrill_component
    temperature_threshold_probe1:
      name: "IGrill target probe 1"
    temperature_threshold_probe2:
      name: "IGrill target probe 2"
    temperature_threshold_probe3:
      name: "IGrill target probe 3"
    temperature_threshold_probe4:
      name: "IGrill target probe 4"
```
> [!NOTE]
> The `number:` platform requires the `IGrill` component to have an explicit
> `id:` set under `sensor:` (see `igrill_component` in the example above), since
> `number:` references it via `igrill_id:` rather than declaring its own
> connection.

## Configuration variables
- **update_interval** (*Optional,* [Time](https://esphome.io/guides/configuration-types.html#config-time)) The interval between each read and publish of sensor values. Defaults to "30s"
- **unit_of_measurement** (*Optional,* string): Manually set the unit of measurement the sensor should advertise its values with. This does not actually do any maths (conversion between units). Defaults to "°C"
- **send_value_when_unplugged** (*Optional,* boolean): When set to `false`, the component will skip publishing for probes that are unplugged. Defaults to `true`
- **unplugged_probe_value** (*Optional,* integer): The value to publish when a probe is disconnected, and **send_value_when_unplugged** is `true`. Defaults to 0

## Available Sensors
- **temperature_probe1** (*Optional*) The reported temperature of probe 1
- **temperature_probe2** (*Optional*) The reported temperature of probe 2
- **temperature_probe3** (*Optional*) The reported temperature of probe 3
- **temperature_probe4** (*Optional*) The reported temperature of probe 4
- **pulse_heating_actual1** (*Optional*) The reported temperature of the left heating element on a Pulse 2000
- **pulse_heating_actual2** (*Optional*) The reported temperature of the right heating element on a Pulse 2000
- **pulse_heating_setpoint1** (*Optional*) The reported setpoint of the left heating element on a Pulse 2000
- **pulse_heating_setpoint2** (*Optional*) The reported setpoint of the right heating element on a Pulse 2000
- **propane_level** (*Optional*) The propane level on a V3 device
- **battery_level** (*Optional*) The battery level of the igrill device

## Available Numbers
Setting one of these writes the value directly to the iGrill over BLE,
which arms the device's own onboard buzzer for that probe — the same as
setting a target from the official app.
 
- **temperature_threshold_probe1** (*Optional*) Target/alarm temperature for probe 1
- **temperature_threshold_probe2** (*Optional*) Target/alarm temperature for probe 2
- **temperature_threshold_probe3** (*Optional*) Target/alarm temperature for probe 3
- **temperature_threshold_probe4** (*Optional*) Target/alarm temperature for probe 4
 
Each accepts the following options:
- **min_value** (*Optional,* float): Defaults to `0`
- **max_value** (*Optional,* float): Defaults to `300`
- **step** (*Optional,* float): Defaults to `1`
 
As with the temperature sensors, values are written/read in whatever unit
the device is currently configured for — no conversion is performed. If your
device is set to °F, override `unit_of_measurement`/`min_value`/`max_value`
accordingly (e.g. `max_value: 572`).
 
If no target has ever been set on a probe (via the app or over BLE), the
number entity will show as `unknown` rather than a placeholder value.

## Additional diagnostic connection sensors
If you require HA sensors to indicate if a BT connection to the iGrill device is established (e.g. for conditional cards), you can use the automations included in `ble_client` to update a template binary sensor like this:

```yaml
ble_client:
  - mac_address: 70:91:8F:XX:XX:XX
    id: igrillv3
    on_connect:
      then:
        - binary_sensor.template.publish:
            id: v3_connection_bin
            state: ON
    on_disconnect:
      then:
        - binary_sensor.template.publish:
            id: v3_connection_bin
            state: OFF

binary_sensor:
  - platform: template
    name: "iGrill V3 connection status"
    id: v3_connection_bin
    device_class: connectivity
    entity_category: diagnostic
```

## Temperature unit:
ESPHome does not support setting sensor temperature unit runtime, so this must be set at compiletime in the YAML config using the `unit_of_measurement` value. This defaults to "°C".

The ESPHome setting is per sensor, but the iGrill has the same for all probes, so if you want to have temperature reported in Farenheit, you need to set `unit_of_measurement: "°F"` on all probes.

You need to set your device to the same unit as you configure in YAML using the iGrill app.

A warning is logged if a mismatch in config is detected.

## Troubleshooting

If the ESPHome device can't connect to your IGrill, please make sure that you disconnect it from any other devices you have used in the past. IGrill devices can't maintain multiple connections.

The same goes the other way around. If you use this component to connect to your IGrill, you can not use the mobile app at the same time.

Also, you can turn the log level up to Verbose to see more diagnostics:

```yaml
logger:
  level: VERBOSE
```

## Disclaimer
This is a work in progress, and some things do not work yet.

What works:
- MAC address discovery with `igrill_ble_listener`
- Connection and authorization
- Detection of model and number of probes
- Publishing of probe temperatures
- Publishing of Pulse 2000 heating element values
- Publishing of battery level
- Publishing of propane level (Untested)
- ~~Use correct temperature unit (read from device)~~
- Read and write temperature setpoint on probes
 
TODO:
- Publish firmware version
- Set temperature unit (write to device)
