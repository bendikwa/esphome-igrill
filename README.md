# esphome-igrill

ESPHome Custom Component for the iGrill Bluetooth Thermometers
This component will let you use a supported ESP32 to read sensor values from IGrill bluetooth thermometers.

## Installation

To use this component, include it as an [External component](https://esphome.io/components/external_components.html)

```yaml
external_components:
  - source: github://bendikwa/esphome-igrill@v1.1
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

Once the device is found, remove the igrill_ble_listener device tracker from your configuration and take note of the device MAC address, and use it when configuring a sensor below.

## Supported Devices
In principle, all IGrill devices, including the Pulse2000 is supported, but I do not own all of them. The ones with a checkmark in the list are confirmed working IGrill models:

- [x] IGrill mini
- [ ] IGrill mini V2
- [x] IGrill V2 - Thanks to [stogs](https://github.com/stogs) for verifying
- [X] IGrill V202
- [x] IGrill V3
- [ ] Weber Pulse2000

If you own one of the untested models, I would be thankfull if you create a ticket so we can get it confirmed working.

## Configuration example

```yaml
esp32_ble_tracker:

ble_client:
  - mac_address: 70:91:8F:XX:XX:XX
    id: igrillv3

sensor:
  - platform: igrill
    ble_client_id: igrillv3
    update_interval: 30s # default
    battery_level:
      name: "IGrill v3 battery"
    temperature_probe1:
      name: "IGrill v3 temp 1"
    temperature_probe2:
      name: "IGrill v3 temp 2"
    temperature_probe3:
      name: "IGrill v3 temp 3"
    temperature_probe4:
      name: "IGrill v3 temp 4"
```
## Configuration variables
- **update_interval** (*Optional,* [Time](https://esphome.io/guides/configuration-types.html#config-time)) The interval between each read and publish of sensor values. Defaults to "30s"
- **send_value_when_unplugged** (*Optional,* boolean): When set to `false`, the component will skip publishing for probes that are unplugged. Defaults to `true`
- **unplugged_probe_value** (*Optional,* integer): The value to publish when a probe is disconnected, and **send_value_when_unplugged** is `true`. Defaults to 0

## Temperature unit:
The temperature unit of the sensors are set to the unit reported by the iGrill device

## Troubleshooting

If the ESPHome device can't connect to your IGrill, please make sure that you disconnect it from any other devices you have used in the past. IGrill devices can't maintain multiple connections.

The same goes the other way around. If you use this component to connect to your IGrill, you can not use the mobile app at the same time.

## Disclaimer
This is a work in progress, and there is a number of things that does not work yet.

What works:
- MAC address discovery with `igrill_ble_listener`
- Connection and authorization
- Detection of model and number of probes
- Publishing of probe temperatures
- Publishing of battery level
- Publishing of propane level (Untested)

TODO:
- Use correct temperature unit (read from device)
- Correctly parse and publish Pulse 2000 heating element probes
- Publish firmware version
- Read and write temperature setpoint on probes
- Set temperature unit (write to device)
