#pragma once
#ifdef USE_ESP32

#include <esp_gattc_api.h>
#include <algorithm>
#include <iterator>
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
namespace esphome {
namespace igrill {


static const char *const SERVICE_UUID = "64AC0000-4A4B-4B58-9F37-94D3C52FFDF7";
static const char *const PROBE1_TEMPERATURE = "06ef0002-2e06-4b79-9e33-fce2c42805ec";
static const char *const BATTERY_LEVEL_UUID = "00002A19-0000-1000-8000-00805F9B34FB";

class IGrill : public PollingComponent, public ble_client::BLEClientNode {
 public:
  IGrill();

  void dump_config() override;
  void update() override;

  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                           esp_ble_gattc_cb_param_t *param) override;

  void set_temperature(sensor::Sensor *temperature) { temperature_sensor_ = temperature; }

 protected:

  void read_battery_(uint8_t *value, uint16_t value_len);
  void request_read_values_();

  sensor::Sensor *temperature_sensor_{nullptr};
  sensor::Sensor *battery_level_sensor{nullptr};

  uint16_t handle_;
  esp32_ble_tracker::ESPBTUUID service_uuid_;
  esp32_ble_tracker::ESPBTUUID probe_1_temperature_uuid_;
  esp32_ble_tracker::ESPBTUUID battery_level_uuid_;

};

}  // namespace igrill
}  // namespace esphome

#endif  // USE_ESP32