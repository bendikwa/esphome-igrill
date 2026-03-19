import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32_ble_tracker

AUTO_LOAD = ["esp32_ble_tracker"]
DEPENDENCIES = ["esp32_ble_tracker"]
CODEOWNERS = ["@bendikwa"]

igrill_ble_listener_ns = cg.esphome_ns.namespace("igrill_ble_listener")
IGrillBLEListener = igrill_ble_listener_ns.class_(
    "IGrillBLEListener", esp32_ble_tracker.ESPBTDeviceListener, cg.Component
)

CONFIG_SCHEMA = cv.Schema(
    {cv.GenerateID(): cv.declare_id(IGrillBLEListener)}
).extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[cv.CONF_ID])
    await cg.register_component(var, config)
    tracker = await cg.get_variable(config[esp32_ble_tracker.CONF_ESP32_BLE_ID])
    cg.add(tracker.register_listener(var))
