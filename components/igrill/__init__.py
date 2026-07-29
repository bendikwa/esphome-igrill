import esphome.codegen as cg
from esphome.components import ble_client

CODEOWNERS = ["@bendikwa"]
DEPENDENCIES = ["ble_client"]

igrill_ns = cg.esphome_ns.namespace("igrill")
IGrill = igrill_ns.class_("IGrill", cg.PollingComponent, ble_client.BLEClientNode)
