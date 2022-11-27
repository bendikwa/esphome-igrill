#include "igrill.h"

#ifdef USE_ESP32

namespace esphome
{
  namespace igrill
  {

    static const char *const TAG = "igrill";

    void IGrill::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                                     esp_ble_gattc_cb_param_t *param)
    {
      switch (event)
      {
      case ESP_GATTC_OPEN_EVT:
      {
        if (param->open.status == ESP_GATT_OK)
        {
          ESP_LOGI(TAG, "Connected successfully!");
        }
        break;
      }

      case ESP_GATTC_CONNECT_EVT:
      {
        ESP_LOGD(TAG, "Setting encryption");
        esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT);
        break;
      }

      case ESP_GATTC_DISCONNECT_EVT:
      {
        ESP_LOGW(TAG, "Disconnected!");
        break;
      }

      case ESP_GATTC_SEARCH_CMPL_EVT:
      {
        model = IGrill::detect_igrill_model_();
        switch (model)
        {
        case IGRILL_MINI:
          if (!(this->probe1_handle_ = get_handle_(IGRILL_MINI_TEMPERATURE_SERVICE_UUID, PROBE1_TEMPERATURE)))
            break;
          break;
        case IGRILLV3:
          if (!(this->probe1_handle_ = get_handle_(IGRILLV3_TEMPERATURE_SERVICE_UUID, PROBE1_TEMPERATURE)))
            break;
          
          if (!(this->probe2_handle_ = get_handle_(IGRILLV3_TEMPERATURE_SERVICE_UUID, PROBE2_TEMPERATURE)))
            break;

          if (!(this->probe3_handle_ = get_handle_(IGRILLV3_TEMPERATURE_SERVICE_UUID, PROBE3_TEMPERATURE)))
            break;

          if (!(this->probe4_handle_ = get_handle_(IGRILLV3_TEMPERATURE_SERVICE_UUID, PROBE4_TEMPERATURE)))
            break;
          break;

        default:
          break;
        }

        if (!(this->battery_level_handle_ = get_handle_(BATTERY_SERVICE_UUID, BATTERY_LEVEL_UUID)))
          break;

        if (!(this->app_challenge_handle_ = get_handle_(AUTHENTICATION_SERVICE_UUID, APP_CHALLENGE_UUID)))
          break;

        if (!(this->device_response_handle_ = get_handle_(AUTHENTICATION_SERVICE_UUID, DEVICE_RESPONSE_UUID)))
          break;

        if (!(this->device_challenge_handle_ = get_handle_(AUTHENTICATION_SERVICE_UUID, DEVICE_CHALLENGE_UUID)))
          break;

        ESP_LOGD(TAG, "Starting autheintication process");
        send_authentication_challenge_();
        this->node_state = esp32_ble_tracker::ClientState::ESTABLISHED;
        break;
      }

      case ESP_GATTC_WRITE_CHAR_EVT:
      {
        if (param->write.conn_id != this->parent()->get_conn_id())
          break;
        if (param->write.status != ESP_GATT_OK)
        {
          ESP_LOGW(TAG, "Error writing char at handle 0x%x, status=%d", param->write.handle, param->write.status);
          break;
        }
        if (param->write.handle == this->app_challenge_handle_)
        {
          ESP_LOGD(TAG, "App challenge written");
          request_device_challenge_read_();
          break;
        }
        else if (param->write.handle == this->device_response_handle_)
        {
          ESP_LOGD(TAG, "Authentication complete");
          request_read_values_();
          break;
        }
      }

      case ESP_GATTC_READ_CHAR_EVT:
      {
        if (param->read.conn_id != this->parent()->get_conn_id())
          break;
        if (param->read.status != ESP_GATT_OK)
        {
          ESP_LOGW(TAG, "Error reading char at handle 0x%x, status=%d", param->read.handle, param->read.status);
          break;
        }
        if (param->read.handle == this->battery_level_handle_)
        {
          read_battery_(param->read.value, param->read.value_len);
          break;
        }
        else if (param->read.handle == this->probe1_handle_)
        {
          read_temperature_(param->read.value, param->read.value_len);
          break;
        }
        else if (param->read.handle == this->device_challenge_handle_)
        {
          loopback_device_challenge_response_(param->read.value, param->read.value_len);
          break;
        }
        break;
      }

      default:
        break;
      }
    }

    IGrillModel IGrill::detect_igrill_model_()
    {
      if (has_service_(IGRILL_MINI_TEMPERATURE_SERVICE_UUID))
      {
        ESP_LOGW(TAG, "Detected model: IGrill mini");
        if (IGrill::has_propane_level_()){
          ESP_LOGW(TAG, "Has propane sensor");
        } else {
          ESP_LOGW(TAG, "Does not have propane sensor");
        }
        return IGRILL_MINI;
      }
      else
      {
        return UNKNOWN;
      }
    }

    bool IGrill::has_service_(const char *service)
    {
      auto *srv = this->parent()->get_service(esp32_ble_tracker::ESPBTUUID::from_raw(service));
      if (srv == nullptr)
      {
        ESP_LOGW(TAG, "No service found at service %s", service);
        return false;
      }
      return true;
    }

    uint16_t IGrill::get_handle_(const char *service, const char *characteristic)
    {
      auto *chr = this->parent()->get_characteristic(esp32_ble_tracker::ESPBTUUID::from_raw(service), esp32_ble_tracker::ESPBTUUID::from_raw(characteristic));
      if (chr == nullptr)
      {
        ESP_LOGW(TAG, "No sensor characteristic found at service %s char %s", service, characteristic);
        return 0;
      }
      return chr->handle;
    }

    void IGrill::read_battery_(uint8_t *raw_value, uint16_t value_len)
    {

      this->battery_level_sensor_->publish_state((float)*raw_value);
    }

    void IGrill::read_temperature_(uint8_t *raw_value, uint16_t value_len)
    {
      uint16_t temp = (raw_value[1] << 8) | raw_value[0];
      this->temperature_sensor_->publish_state((float)temp);
    }

    void IGrill::update()
    {
      if (this->node_state != esp32_ble_tracker::ClientState::ESTABLISHED)
      {
        if (!parent()->enabled)
        {
          ESP_LOGW(TAG, "Reconnecting to device");
          parent()->set_enabled(true);
          parent()->connect();
        }
        else
        {
          ESP_LOGW(TAG, "Connection in progress");
        }
      }
    }

    void IGrill::send_authentication_challenge_()
    {
      uint8_t zero_payload[16] = {};
      ;
      ESP_LOGD(TAG, "Sending app challenge with all '0's");
      auto status = esp_ble_gattc_write_char(this->parent()->get_gattc_if(), this->parent()->get_conn_id(), app_challenge_handle_, sizeof(zero_payload), zero_payload, ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);

      if (status)
      {
        ESP_LOGW(TAG, "Error writing challenge to app challenge characteristic, status=%d", status);
      }
    }

    void IGrill::loopback_device_challenge_response_(uint8_t *raw_value, uint16_t value_len)
    {

      ESP_LOGD(TAG, "Sending encrypted device response back to device.");
      auto status = esp_ble_gattc_write_char(this->parent()->get_gattc_if(), this->parent()->get_conn_id(), device_response_handle_, value_len, raw_value, ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);

      if (status)
      {
        ESP_LOGW(TAG, "Error writing challenge response to device response characteristic, status=%d", status);
      }
    }

    void IGrill::request_device_challenge_read_()
    {
      ESP_LOGD(TAG, "Requesting read of encrypted device response from device");
      auto status = esp_ble_gattc_read_char(this->parent()->get_gattc_if(), this->parent()->get_conn_id(), this->device_challenge_handle_, ESP_GATT_AUTH_REQ_NONE);
      if (status)
      {
        ESP_LOGW(TAG, "Error sending read request for device_challenge, status=%d", status);
      }
    }

    void IGrill::request_read_values_()
    {
      auto status = esp_ble_gattc_read_char(this->parent()->get_gattc_if(), this->parent()->get_conn_id(), this->battery_level_handle_, ESP_GATT_AUTH_REQ_NONE);
      if (status)
      {
        ESP_LOGW(TAG, "Error sending read request for sensor, status=%d", status);
      }

      status = esp_ble_gattc_read_char(this->parent()->get_gattc_if(), this->parent()->get_conn_id(), this->probe1_handle_, ESP_GATT_AUTH_REQ_NONE);
      if (status)
      {
        ESP_LOGW(TAG, "Error sending read request for sensor, status=%d", status);
      }
    }

    void IGrill::dump_config()
    {
      LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
      LOG_SENSOR("  ", "Battery Level", this->battery_level_sensor_);
    }

    IGrill::IGrill()
        : PollingComponent(10000) {}

  } // namespace igrill
} // namespace esphome

#endif // USE_ESP32