#pragma once
#ifdef USE_ESP32

#include <esp_gattc_api.h>
#include <algorithm>
#include <iterator>
#include <map>
#include <string>
#include <vector>
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"

namespace esphome
{
  namespace igrill
  {

    static const char *const AUTHENTICATION_SERVICE_UUID = "64AC0000-4A4B-4B58-9F37-94D3C52FFDF7";
    static const char *const APP_CHALLENGE_UUID = "64AC0002-4A4B-4B58-9F37-94D3C52FFDF7";
    static const char *const DEVICE_CHALLENGE_UUID = "64AC0003-4A4B-4B58-9F37-94D3C52FFDF7";
    static const char *const DEVICE_RESPONSE_UUID = "64AC0004-4A4B-4B58-9F37-94D3C52FFDF7";

    static const char *const IGRILL_MINI_TEMPERATURE_SERVICE_UUID = "63C70000-4A82-4261-95FF-92CF32477861";
    static const char *const IDEVICES_KITCHEN_TEMPERATURE_SERVICE_UUID = "19450000-9B05-40BB-80D8-7C85840AEC34";
    static const char *const IGRILL_MINIV2_TEMPERATURE_SERVICE_UUID = "9d610c43-ae1d-41a9-9b09-3c7ecd5c6035";
    static const char *const IGRILLV2_TEMPERATURE_SERVICE_UUID = "A5C50000-F186-4BD6-97F2-7EBACBA0D708";
    static const char *const IGRILLV202_TEMPERATURE_SERVICE_UUID = "ADA7590F-2E6D-469E-8F7B-1822B386A5E9";
    static const char *const IGRILLV3_TEMPERATURE_SERVICE_UUID = "6E910000-58DC-41C7-943F-518B278CEA88";
    static const char *const PULSE_1000_TEMPERATURE_SERVICE_UUID = "7E920000-68DC-41C7-943F-518B278CEA87";
    static const char *const PULSE_2000_TEMPERATURE_SERVICE_UUID = "7E920000-68DC-41C7-943F-518B278CEA88";
    static const char *const PULSE_ELEMENT_SERVICE_UUID = "6c910000-58dc-41c7-943f-518b278ceaaa";
    static const char *const TEMPERATURE_UNIT_UUID = "06ef0001-2e06-4b79-9e33-fce2c42805ec";
    static const char *const PROBE1_TEMPERATURE = "06ef0002-2e06-4b79-9e33-fce2c42805ec";
    static const char *const PROBE2_TEMPERATURE = "06ef0004-2e06-4b79-9e33-fce2c42805ec";
    static const char *const PROBE3_TEMPERATURE = "06ef0006-2e06-4b79-9e33-fce2c42805ec";
    static const char *const PROBE4_TEMPERATURE = "06ef0008-2e06-4b79-9e33-fce2c42805ec";
    static const char *const PULSE_ELEMENT_UUID = "6c91000a-58dc-41c7-943f-518b278ceaaa";
    
    static const char *const PROPANE_LEVEL_SERVICE_UUID = "F5D40000-3548-4C22-9947-F3673FCE3CD9";
    static const char *const PROPANE_LEVEL = "f5d40001-3548-4c22-9947-f3673fce3cd9";

    static const char *const BATTERY_SERVICE_UUID = "180F";
    static const char *const BATTERY_LEVEL_UUID = "2A19";

    static const uint16_t UNPLUGGED_PROBE_CONSTANT = 63536;
    
    static const char *const FAHRENHEIT_UNIT_STRING = "°F";
    static const char *const CELCIUS_UNIT_STRING = "°C";

    class IGrill : public PollingComponent, public ble_client::BLEClientNode
    {
    public:
      IGrill();

      void dump_config() override;
      void update() override;

      void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) override;
      void set_temperature_probe(sensor::Sensor *temperature, int probe_num) { sensors_[probe_num-1] = temperature; }
      void set_pulse_actual1(sensor::Sensor *pulse_actual) { pulse_heating_actual1_ = pulse_actual; }
      void set_pulse_actual2(sensor::Sensor *pulse_actual) { pulse_heating_actual2_ = pulse_actual; }
      void set_pulse_setpoint1(sensor::Sensor *pulse_setpoint) { pulse_heating_setpoint1_ = pulse_setpoint; }
      void set_pulse_setpoint2(sensor::Sensor *pulse_setpoint) { pulse_heating_setpoint2_ = pulse_setpoint; }
      void set_propane(sensor::Sensor *propane) { propane_level_sensor_ = propane; }
      void set_battery(sensor::Sensor *battery) { battery_level_sensor_ = battery; }
      void set_send_value_when_unplugged(bool send_value_when_unplugged) { ESP_LOGE("igrill", "send_value_when_unplugged: %d", send_value_when_unplugged); send_value_when_unplugged_ = send_value_when_unplugged; }
      void set_unplugged_probe_value(float unplugged_probe_value) {unplugged_probe_value_ = unplugged_probe_value; }

    protected:
      void detect_and_init_igrill_model_();
      bool has_service_(const char *service);
      bool is_same_address_(uint8_t *a, uint8_t *b);
      void add_temperature_probe_handles_(const char *service);
      uint16_t get_handle_(const char *service, const char *chr);
      void read_battery_(uint8_t *value, uint16_t value_len);
      void read_temperature_unit_(uint8_t *value, uint16_t value_len);
      void read_propane_(uint8_t *value, uint16_t value_len);
      void read_pulse_element_(uint8_t *value, uint16_t value_len);
      void read_temperature1_(uint8_t *value, uint16_t value_len){ read_temperature_(value, value_len, 0); }
      void read_temperature2_(uint8_t *value, uint16_t value_len){ read_temperature_(value, value_len, 1); }
      void read_temperature3_(uint8_t *value, uint16_t value_len){ read_temperature_(value, value_len, 2); }
      void read_temperature4_(uint8_t *value, uint16_t value_len){ read_temperature_(value, value_len, 3); }
      void read_temperature_(uint8_t *value, uint16_t value_len, int probe);
      void request_read_values_();
      void request_temp_unit_read_();
      void request_device_challenge_read_();
      void send_authentication_challenge_();
      void loopback_device_challenge_response_(uint8_t *raw_value, uint16_t value_len);

      int num_probes = 0;
      bool send_value_when_unplugged_;
      float unplugged_probe_value_;
      const char *unit_of_measurement_{nullptr};
      
      std::vector<sensor::Sensor *> sensors_ = {nullptr, nullptr, nullptr, nullptr};
      sensor::Sensor *battery_level_sensor_{nullptr};
      sensor::Sensor *propane_level_sensor_{nullptr};
      sensor::Sensor *pulse_heating_actual1_{nullptr};
      sensor::Sensor *pulse_heating_actual2_{nullptr};
      sensor::Sensor *pulse_heating_setpoint1_{nullptr};
      sensor::Sensor *pulse_heating_setpoint2_{nullptr};

      uint16_t app_challenge_handle_;
      uint16_t battery_level_handle_;
      uint16_t device_challenge_handle_;
      uint16_t device_response_handle_;
      std::vector<uint16_t> probe_handles_;
      uint16_t propane_level_handle_;
      uint16_t pulse_element_handle_;
      std::map<uint16_t, void (esphome::igrill::IGrill::*)(uint8_t *, uint16_t)> value_readers_;
      uint16_t temperature_unit_handle_;
    };

  } // namespace igrill
} // namespace esphome

#endif // USE_ESP32
