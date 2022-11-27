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

namespace esphome
{
  namespace igrill
  {

    static const char *const AUTHENTICATION_SERVICE_UUID = "64AC0000-4A4B-4B58-9F37-94D3C52FFDF7";
    static const char *const APP_CHALLENGE_UUID = "64AC0002-4A4B-4B58-9F37-94D3C52FFDF7";
    static const char *const DEVICE_CHALLENGE_UUID = "64AC0003-4A4B-4B58-9F37-94D3C52FFDF7";
    static const char *const DEVICE_RESPONSE_UUID = "64AC0004-4A4B-4B58-9F37-94D3C52FFDF7";

    static const char *const IGRILL_MINI_TEMPERATURE_SERVICE_UUID = "63C70000-4A82-4261-95FF-92CF32477861";
    static const char *const IGRILL_MINIV2_TEMPERATURE_SERVICE_UUID = "9d610c43-ae1d-41a9-9b09-3c7ecd5c6035";
    static const char *const IGRILLV2_TEMPERATURE_SERVICE_UUID = "A5C50000-F186-4BD6-97F2-7EBACBA0D708";
    static const char *const IGRILLV202_TEMPERATURE_SERVICE_UUID = "ADA7590F-2E6D-469E-8F7B-1822B386A5E9";
    static const char *const IGRILLV3_TEMPERATURE_SERVICE_UUID = "6E910000-58DC-41C7-943F-518B278CEA88";
    static const char *const PROBE1_TEMPERATURE = "06ef0002-2e06-4b79-9e33-fce2c42805ec";
    static const char *const PROBE2_TEMPERATURE = "06ef0004-2e06-4b79-9e33-fce2c42805ec";
    static const char *const PROBE3_TEMPERATURE = "06ef0006-2e06-4b79-9e33-fce2c42805ec";
    static const char *const PROBE4_TEMPERATURE = "06ef0008-2e06-4b79-9e33-fce2c42805ec";

    static const char *const BATTERY_SERVICE_UUID = "180F";
    static const char *const BATTERY_LEVEL_UUID = "2A19";

    enum IGrillModel { IGRILL_MINI, IGRILL_MINI_V2, IGRILLV3, UNKNOWN };

    class IGrill : public PollingComponent, public ble_client::BLEClientNode
    {
    public:
      IGrill();

      void dump_config() override;
      void update() override;

      void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) override;
      void set_temperature(sensor::Sensor *temperature) { temperature_sensor_ = temperature; }
      void set_battery(sensor::Sensor *battery) { battery_level_sensor_ = battery; }
      void set_has_propane_level(bool has_propane_level) { propane_level_ = has_propane_level; }

    protected:
      IGrillModel detect_igrill_model_();
      bool has_propane_level_() { return propane_level_;}
      bool has_service_(const char * service);
      uint16_t get_handle_(const char * service, const char * chr);
      void read_battery_(uint8_t *value, uint16_t value_len);
      void read_temperature_(uint8_t *value, uint16_t value_len);
      void request_read_values_();
      void request_device_challenge_read_();
      void send_authentication_challenge_();
      void loopback_device_challenge_response_(uint8_t *raw_value, uint16_t value_len);

      IGrillModel model = UNKNOWN;
      bool propane_level_;

      sensor::Sensor *temperature_sensor_{nullptr};
      sensor::Sensor *battery_level_sensor_{nullptr};

      uint16_t app_challenge_handle_;
      uint16_t device_challenge_handle_;
      uint16_t device_response_handle_;
      uint16_t probe1_handle_;
      uint16_t probe2_handle_;
      uint16_t probe3_handle_;
      uint16_t probe4_handle_;
      uint16_t battery_level_handle_;
    };

  } // namespace igrill
} // namespace esphome

#endif // USE_ESP32