import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, ble_client

from esphome.const import (
    CONF_BATTERY_LEVEL,
    CONF_ID,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_TEMPERATURE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_PERCENT,
    UNIT_CELSIUS,
)

DEPENDENCIES = ["ble_client"]

igrill_ns = cg.esphome_ns.namespace("igrill")
IGrill = igrill_ns.class_(
    "IGrill", cg.PollingComponent, ble_client.BLEClientNode
)
CONF_SEND_VALUE_WHEN_UNPLUGGED = "send_value_when_unplugged"
CONF_UNPLUGGED_PROBE_VALUE = "unplugged_probe_value"
CONF_PROPANE_LEVEL = "propane_level"
CONF_TEMPERATURE_PROBE1 = "temperature_probe1"
CONF_TEMPERATURE_PROBE2 = "temperature_probe2"
CONF_TEMPERATURE_PROBE3 = "temperature_probe3"
CONF_TEMPERATURE_PROBE4 = "temperature_probe4"
CONF_PULSE_ACTUAL1 = "pulse_heating_actual1"
CONF_PULSE_ACTUAL2 = "pulse_heating_actual2"
CONF_PULSE_SETPOINT1 = "pulse_heating_setpoint1"
CONF_PULSE_SETPOINT2 = "pulse_heating_setpoint2"

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(IGrill),
            cv.Optional(CONF_BATTERY_LEVEL): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_BATTERY,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_TEMPERATURE_PROBE1): sensor.sensor_schema(
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_TEMPERATURE_PROBE2): sensor.sensor_schema(
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_TEMPERATURE_PROBE3): sensor.sensor_schema(
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_TEMPERATURE_PROBE4): sensor.sensor_schema(
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_PULSE_ACTUAL1): sensor.sensor_schema(
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_PULSE_ACTUAL2): sensor.sensor_schema(
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_PULSE_SETPOINT1): sensor.sensor_schema(
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_PULSE_SETPOINT2): sensor.sensor_schema(
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_PROPANE_LEVEL): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_SEND_VALUE_WHEN_UNPLUGGED, default=True): cv.boolean,
            cv.Optional(CONF_UNPLUGGED_PROBE_VALUE, default=0): cv.float_,
        }
    )
    .extend(cv.polling_component_schema("30s"))
    .extend(ble_client.BLE_CLIENT_SCHEMA),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    await ble_client.register_ble_node(var, config)

    if CONF_BATTERY_LEVEL in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY_LEVEL])
        cg.add(var.set_battery(sens))
    if CONF_TEMPERATURE_PROBE1 in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE_PROBE1])
        cg.add(var.set_temperature_probe(sens, 1))
    if CONF_TEMPERATURE_PROBE2 in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE_PROBE2])
        cg.add(var.set_temperature_probe(sens, 2))
    if CONF_TEMPERATURE_PROBE3 in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE_PROBE3])
        cg.add(var.set_temperature_probe(sens, 3))
    if CONF_TEMPERATURE_PROBE4 in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE_PROBE4])
        cg.add(var.set_temperature_probe(sens, 4))
    if CONF_PULSE_ACTUAL1 in config:
        sens = await sensor.new_sensor(config[CONF_PULSE_ACTUAL1])
        cg.add(var.set_pulse_actual1(sens))
    if CONF_PULSE_ACTUAL2 in config:
        sens = await sensor.new_sensor(config[CONF_PULSE_ACTUAL2])
        cg.add(var.set_pulse_actual2(sens))
    if CONF_PULSE_SETPOINT1 in config:
        sens = await sensor.new_sensor(config[CONF_PULSE_SETPOINT1])
        cg.add(var.set_pulse_setpoint1(sens))
    if CONF_PULSE_SETPOINT2 in config:
        sens = await sensor.new_sensor(config[CONF_PULSE_SETPOINT2])
        cg.add(var.set_pulse_setpoint2(sens))
    if CONF_PROPANE_LEVEL in config:
        sens = await sensor.new_sensor(config[CONF_PROPANE_LEVEL])
        cg.add(var.set_propane(sens))
    cg.add(var.set_send_value_when_unplugged(config[CONF_SEND_VALUE_WHEN_UNPLUGGED]))
    cg.add(var.set_unplugged_probe_value(config[CONF_UNPLUGGED_PROBE_VALUE]))