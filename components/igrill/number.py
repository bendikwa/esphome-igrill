import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number

from esphome.const import (
    CONF_ID,
    CONF_MIN_VALUE,
    CONF_MAX_VALUE,
    CONF_STEP,
    DEVICE_CLASS_TEMPERATURE,
    ENTITY_CATEGORY_CONFIG,
    UNIT_CELSIUS,
)

from . import igrill_ns, IGrill

CONF_IGRILL_ID = "igrill_id"
CONF_TEMPERATURE_THRESHOLD_PROBE1 = "temperature_threshold_probe1"
CONF_TEMPERATURE_THRESHOLD_PROBE2 = "temperature_threshold_probe2"
CONF_TEMPERATURE_THRESHOLD_PROBE3 = "temperature_threshold_probe3"
CONF_TEMPERATURE_THRESHOLD_PROBE4 = "temperature_threshold_probe4"

THRESHOLD_CONFS = [
    CONF_TEMPERATURE_THRESHOLD_PROBE1,
    CONF_TEMPERATURE_THRESHOLD_PROBE2,
    CONF_TEMPERATURE_THRESHOLD_PROBE3,
    CONF_TEMPERATURE_THRESHOLD_PROBE4,
]

IGrillThresholdNumber = igrill_ns.class_("IGrillThresholdNumber", number.Number, cg.Component)


def _threshold_schema():
    # Defaults assume °C (0-300, step 1). If your probes are configured for °F
    # in the iGrill app, override unit_of_measurement/min_value/max_value per
    # entry, e.g. max_value: 572
    return number.number_schema(
        IGrillThresholdNumber,
        unit_of_measurement=UNIT_CELSIUS,
        device_class=DEVICE_CLASS_TEMPERATURE,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:thermometer-alert",
    ).extend(
        {
            cv.Optional(CONF_MIN_VALUE, default=0): cv.float_,
            cv.Optional(CONF_MAX_VALUE, default=300): cv.float_,
            cv.Optional(CONF_STEP, default=1): cv.float_,
        }
    )


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_IGRILL_ID): cv.use_id(IGrill),
        cv.Optional(CONF_TEMPERATURE_THRESHOLD_PROBE1): _threshold_schema(),
        cv.Optional(CONF_TEMPERATURE_THRESHOLD_PROBE2): _threshold_schema(),
        cv.Optional(CONF_TEMPERATURE_THRESHOLD_PROBE3): _threshold_schema(),
        cv.Optional(CONF_TEMPERATURE_THRESHOLD_PROBE4): _threshold_schema(),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_IGRILL_ID])

    for probe_num, conf_key in enumerate(THRESHOLD_CONFS, start=1):
        if conf_key not in config:
            continue
        conf = config[conf_key]
        num = await number.new_number(
            conf,
            min_value=conf[CONF_MIN_VALUE],
            max_value=conf[CONF_MAX_VALUE],
            step=conf[CONF_STEP],
        )
        await cg.register_component(num, conf)
        cg.add(parent.set_temperature_threshold(num, probe_num))
