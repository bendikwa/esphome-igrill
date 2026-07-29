# Adding read/write target temperature to esphome-igrill

`bendikwa/esphome-igrill` currently only reads probe temperatures. Setting a
target (so the iGrill's own buzzer fires) is listed in its README as a TODO
and isn't implemented. This patch adds it as a new `number` platform.

## What changed

- **`components/igrill/igrill.h` / `igrill.cpp`**
  - Added the 4 "threshold" BLE characteristic UUIDs (`06ef0003`, `06ef0005`,
    `06ef0007`, `06ef0009` — one per probe, sitting right after each probe's
    temperature characteristic `06ef0002/4/6/8`). These hold the alarm/target
    temperature and are both readable and writable.
  - Added `IGrillThresholdNumber`, a `number::Number` entity tied back to the
    `IGrill` instance.
  - Added `read_threshold_()` to parse readback the same way probe
    temperatures are parsed (2-byte little-endian, no scaling).
  - Added `write_threshold()`, which does a GATT characteristic write of the
    new value (also 2-byte little-endian) — this is what pushes the value to
    the physical device and arms its buzzer.
  - Threshold characteristics are now polled every `update()` cycle alongside
    temperatures/battery, so the entity in Home Assistant reflects whatever
    the grill currently has stored (including values set from the iGrill app
    itself).

- **`components/igrill/number.py`** (new file) — the ESPHome config platform
  for `number: - platform: igrill`.

- **`components/igrill/__init__.py`** — now holds the shared `IGrill` class
  reference so both `sensor.py` and `number.py` can use it.

- **`components/igrill/sensor.py`** — unchanged in behavior, just imports
  `IGrill` from `__init__.py` instead of declaring it twice.

## Using it

See `example_with_targets.yaml`. Key points:

1. Give the `IGrill` component an explicit `id:` under the `sensor:` platform
   block — the `number:` platform needs to reference it via `igrill_id:`.
2. Values are in whatever unit the iGrill itself is currently set to (same as
   the temperature sensors) — no conversion is done in software. If your
   probes report °F, override `unit_of_measurement`, `min_value`, and
   `max_value` on the threshold entries accordingly (e.g. `max_value: 572`).
3. Because iGrill devices only support **one active BLE connection**, setting
   a target from Home Assistant will only work while the ESP32 is connected —
   and you can't have the iGrill app open at the same time.

## Caveats / things to verify on your hardware

- I derived the threshold characteristic UUIDs and 2-byte little-endian
  write format from the author's earlier `bendikwa/igrill` (Python/bluepy)
  project, which used the same BLE service and read the thresholds the same
  way temperatures are read. I have not had physical hardware to test the
  **write** path end-to-end, so:
  - Watch the ESPHome logs (`logger: level: VERBOSE`) the first time you set
    a value — you should see `Writing threshold ... status=0` and then, on
    the next poll, the readback should match what you set.
  - If the write silently fails or the value doesn't stick, it's possible
    some models require writing through a different characteristic property
    (e.g. "write without response" instead of "write with response") — try
    changing `ESP_GATT_WRITE_TYPE_RSP` to `ESP_GATT_WRITE_TYPE_NO_RSP` in
    `write_threshold()` in that case.
- This only targets iGrill mini/V2/V202/V3-style probes (characteristics
  `06ef000X`). The Weber Pulse 2000's heating-element setpoints use a
  completely different characteristic (`6c91000a...`, parsed as ASCII digits
  in `read_pulse_element_`) and are not covered by this patch.

## Suggestion

Once you've confirmed this works on your device, consider opening a PR
against `bendikwa/esphome-igrill` — this is exactly the TODO item listed in
their README, and the maintainer would probably welcome it.
