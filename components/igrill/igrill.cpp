#include "igrill.h"

#ifdef USE_ESP32

namespace esphome {
namespace igrill {

static const char *const TAG = "igrill";

void IGrill::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                                            esp_ble_gattc_cb_param_t *param) {
  switch (event) {
    case ESP_GATTC_OPEN_EVT: {
      if (param->open.status == ESP_GATT_OK) {
        ESP_LOGI(TAG, "Connected successfully!");
      }
      break;
    }

    case ESP_GATTC_DISCONNECT_EVT: {
      ESP_LOGW(TAG, "Disconnected!");
      break;
    }

    case ESP_GATTC_SEARCH_CMPL_EVT: {
      this->handle_ = 0;
      auto *chr = this->parent()->get_characteristic(service_uuid_, battery_level_uuid_);
      if (chr == nullptr) {
        ESP_LOGW(TAG, "No sensor characteristic found at service %s char %s", service_uuid_.to_string().c_str(),
                 sensors_data_characteristic_uuid_.to_string().c_str());
        break;
      }
      this->handle_ = chr->handle;
      this->node_state = esp32_ble_tracker::ClientState::ESTABLISHED;

      request_read_values_();
      break;
    }

    case ESP_GATTC_READ_CHAR_EVT: {
      if (param->read.conn_id != this->parent()->get_conn_id())
        break;
      if (param->read.status != ESP_GATT_OK) {
        ESP_LOGW(TAG, "Error reading char at handle %d, status=%d", param->read.handle, param->read.status);
        break;
      }
      if (param->read.handle == this->handle_) {
        read_battery_(param->read.value, param->read.value_len);
      }
      break;
    }

    default:
      break;
  }
}

void IGrill::read_battery_(uint8_t *raw_value, uint16_t value_len) {

    this->battery_level_sensor->publish_state((float) *raw_value);

    // This instance must not stay connected
    // so other clients can connect to it (e.g. the
    // mobile app).
    parent()->set_enabled(false);
}


void IGrill::update() {
  if (this->node_state != esp32_ble_tracker::ClientState::ESTABLISHED) {
    if (!parent()->enabled) {
      ESP_LOGW(TAG, "Reconnecting to device");
      parent()->set_enabled(true);
      parent()->connect();
    } else {
      ESP_LOGW(TAG, "Connection in progress");
    }
  }
}

void IGrill::request_read_values_() {
  auto status = esp_ble_gattc_read_char(this->parent()->get_gattc_if(), this->parent()->get_conn_id(), this->handle_,
                                        ESP_GATT_AUTH_REQ_NONE);
  if (status) {
    ESP_LOGW(TAG, "Error sending read request for sensor, status=%d", status);
  }
}

void IGrill::dump_config() {
  LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
}

IGrill::IGrill()
    : PollingComponent(10000),
      service_uuid_(esp32_ble_tracker::ESPBTUUID::from_raw(SERVICE_UUID)),
      probe_1_temperature_uuid_(esp32_ble_tracker::ESPBTUUID::from_raw(PROBE1_TEMPERATURE)),
      battery_level_uuid_(esp32_ble_tracker::ESPBTUUID::from_raw(BATTERY_LEVEL_UUID)) {}

}  // namespace igrill
}  // namespace esphome

#endif  // USE_ESP32