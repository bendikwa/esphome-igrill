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
        if (!is_same_address_(param->connect.remote_bda, this->parent()->get_remote_bda()))
        {
          ESP_LOGD(TAG, "This ESP_GATTC_CONNECT_EVT is not for me. (remote_bda mismatch");
          break;
        }
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
        if (param->search_cmpl.conn_id != this->parent()->get_conn_id())
        {
          ESP_LOGD(TAG, "This ESP_GATTC_SEARCH_CMPL_EVT is not for me. (conn_id mismatch");
          break;
        }
        // Detect IGrill model and get hadles for the appropriate number of probes.
        IGrill::detect_and_init_igrill_model_();

        // Get handle for battery level
        this->battery_level_handle_ = get_handle_(BATTERY_SERVICE_UUID, BATTERY_LEVEL_UUID);

        // Get handle for propane level, if configured
        if (this->propane_level_sensor_ != nullptr)
        {
          this->propane_level_handle_ = get_handle_(PROPANE_LEVEL_SERVICE_UUID, PROPANE_LEVEL);
        }

        // Get handles for authentication
        this->app_challenge_handle_ = get_handle_(AUTHENTICATION_SERVICE_UUID, APP_CHALLENGE_UUID);
        this->device_response_handle_ = get_handle_(AUTHENTICATION_SERVICE_UUID, DEVICE_RESPONSE_UUID);
        this->device_challenge_handle_ = get_handle_(AUTHENTICATION_SERVICE_UUID, DEVICE_CHALLENGE_UUID);

        ESP_LOGD(TAG, "Starting autheintication process");
        send_authentication_challenge_();

        this->node_state = esp32_ble_tracker::ClientState::ESTABLISHED;
        break;
      }

      case ESP_GATTC_WRITE_CHAR_EVT:
      {
        if (param->write.conn_id != this->parent()->get_conn_id())
        {
          ESP_LOGD(TAG, "This ESP_GATTC_WRITE_CHAR_EVT is not for me. (conn_id mismatch");
          break;
        }
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
          request_temp_unit_read_();
          break;
        }
      }

      case ESP_GATTC_READ_CHAR_EVT:
      {
        if (param->read.conn_id != this->parent()->get_conn_id())
        {
          ESP_LOGD(TAG, "This ESP_GATTC_READ_CHAR_EVT is not for me. (conn_id mismatch");
          break;
        }
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
        else if (param->read.handle == this->propane_level_handle_)
        {
          read_propane_(param->read.value, param->read.value_len);
          break;
        }
        else if (param->read.handle == this->probe1_handle_)
        {
          read_temperature_(param->read.value, param->read.value_len, 1);
          break;
        }
        else if (param->read.handle == this->probe2_handle_)
        {
          read_temperature_(param->read.value, param->read.value_len, 2);
          break;
        }
        else if (param->read.handle == this->probe3_handle_)
        {
          read_temperature_(param->read.value, param->read.value_len, 3);
          break;
        }
        else if (param->read.handle == this->probe4_handle_)
        {
          read_temperature_(param->read.value, param->read.value_len, 4);
          break;
        }
        else if (param->read.handle == this->temperature_unit_handle_)
        {
          read_temperature_unit_(param->read.value, param->read.value_len);
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

    void IGrill::detect_and_init_igrill_model_()
    {
      const char *service;
      if (has_service_(IGRILL_MINI_TEMPERATURE_SERVICE_UUID))
      {
        ESP_LOGI(TAG, "Detected model: IGrill mini");
        num_probes = 1;
        service = IGRILL_MINI_TEMPERATURE_SERVICE_UUID;
      }
      else if (has_service_(IGRILL_MINIV2_TEMPERATURE_SERVICE_UUID))
      {
        ESP_LOGI(TAG, "Detected model: IGrill mini V2");
        num_probes = 1;
        service = IGRILL_MINIV2_TEMPERATURE_SERVICE_UUID;
      }
      else if (has_service_(IGRILLV2_TEMPERATURE_SERVICE_UUID))
      {
        ESP_LOGI(TAG, "Detected model: IGrill V2");
        num_probes = 4;
        service = IGRILLV2_TEMPERATURE_SERVICE_UUID;
      }
      else if (has_service_(IGRILLV202_TEMPERATURE_SERVICE_UUID))
      {
        ESP_LOGI(TAG, "Detected model: IGrill V202");
        num_probes = 4;
        service = IGRILLV202_TEMPERATURE_SERVICE_UUID;
      }
      else if (has_service_(IGRILLV3_TEMPERATURE_SERVICE_UUID))
      {
        ESP_LOGI(TAG, "Detected model: IGrill V3");
        num_probes = 4;
        service = IGRILLV3_TEMPERATURE_SERVICE_UUID;
      }
      else
      {
        ESP_LOGE(TAG, "Could not identify IGrill model");
        return;
      }
      IGrill::get_temperature_probe_handles_(service);
      this->temperature_unit_handle_ = get_handle_(service, TEMPERATURE_UNIT_UUID);
    }

    bool IGrill::has_service_(const char *service)
    {
      auto *srv = this->parent()->get_service(esp32_ble_tracker::ESPBTUUID::from_raw(service));
      if (srv == nullptr)
      {
        ESP_LOGD(TAG, "No service found at service %s", service);
        return false;
      }
      return true;
    }

    void IGrill::get_temperature_probe_handles_(const char *service)
    {
      std::vector<const char *> probes = {PROBE1_TEMPERATURE, PROBE2_TEMPERATURE, PROBE3_TEMPERATURE, PROBE4_TEMPERATURE};
      for (int i = 0; i < this->num_probes; i++)
      {
        *this->handles[i] = get_handle_(service, probes[i]);
      }
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

    void IGrill::read_propane_(uint8_t *raw_value, uint16_t value_len)
    {

      this->propane_level_sensor_->publish_state(((float)*raw_value * 25));
    }

    void IGrill::read_temperature_unit_(uint8_t *raw_value, uint16_t value_len)
    {
      if (raw_value[0] == 0)
      {
        this->unit_of_measurement_ = FAHRENHEIT_UNIT_STRING;
      }
      else
      {
        this->unit_of_measurement_ = CELCIUS_UNIT_STRING;
      }
      ESP_LOGI(TAG, "Setting temperature unit based on device: %s", this->unit_of_measurement_.c_str());

      if (this->temperature_probe1_sensor_ != nullptr)
      {
        temperature_probe1_sensor_->set_unit_of_measurement(this->unit_of_measurement_);
      }
      if (this->temperature_probe2_sensor_ != nullptr)
      {
        temperature_probe2_sensor_->set_unit_of_measurement(this->unit_of_measurement_);
      }
      if (this->temperature_probe3_sensor_ != nullptr)
      {
        temperature_probe3_sensor_->set_unit_of_measurement(this->unit_of_measurement_);
      }
      if (this->temperature_probe4_sensor_ != nullptr)
      {
        temperature_probe4_sensor_->set_unit_of_measurement(this->unit_of_measurement_);
      }
      request_read_values_();
    }

    void IGrill::read_temperature_(uint8_t *raw_value, uint16_t value_len, int probe)
    {
      uint16_t raw_temp = (raw_value[1] << 8) | raw_value[0];
      bool probe_unplugged = raw_temp == UNPLUGGED_PROBE_CONSTANT;
      bool publish = true;
      float temp = (float)raw_temp;
      if (probe_unplugged)
      {
        temp = this->unplugged_probe_value_;
        publish = send_value_when_unplugged_;
      }
      if (publish)
      {
        switch (probe)
        {
        case 1:
          this->temperature_probe1_sensor_->publish_state(temp);
          break;

        case 2:
          this->temperature_probe2_sensor_->publish_state(temp);
          break;

        case 3:
          this->temperature_probe3_sensor_->publish_state(temp);
          break;

        case 4:
          this->temperature_probe4_sensor_->publish_state(temp);
          break;

        default:
          break;
        }
      }
    }

    // TODO Hande update()
    void IGrill::update()
    {
      switch (this->node_state)
      {
      case esp32_ble_tracker::ClientState::ESTABLISHED:
        if (this->unit_of_measurement_.empty())
        {
          ESP_LOGD(TAG, "Requesting read of temperature unit");
          request_temp_unit_read_();
        }
        else
        {
          ESP_LOGD(TAG, "Requesting read of all probe values");
          request_read_values_();
        }
        break;

      default:
        ESP_LOGD(TAG, "No IGrill device connected");
        break;
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

    void IGrill::request_temp_unit_read_()
    {
      auto status = esp_ble_gattc_read_char(this->parent()->get_gattc_if(), this->parent()->get_conn_id(), this->temperature_unit_handle_, ESP_GATT_AUTH_REQ_NONE);
      if (status)
      {
        ESP_LOGW(TAG, "Error sending read request for temperature unit, status=%d", status);
      }
    }

    void IGrill::request_read_values_()
    {
      // Read battery level
      auto status = esp_ble_gattc_read_char(this->parent()->get_gattc_if(), this->parent()->get_conn_id(), this->battery_level_handle_, ESP_GATT_AUTH_REQ_NONE);
      if (status)
      {
        ESP_LOGW(TAG, "Error sending read request for sensor, status=%d", status);
      }

      // Read temperature probes
      for (int i = 0; i < this->num_probes; i++)
      {
        status = esp_ble_gattc_read_char(this->parent()->get_gattc_if(), this->parent()->get_conn_id(), *this->handles[i], ESP_GATT_AUTH_REQ_NONE);
        if (status)
        {
          ESP_LOGW(TAG, "Error sending read request for sensor, status=%d", status);
        }
      }

      // Read propane level
      if (this->propane_level_sensor_ != nullptr)
      {
        status = esp_ble_gattc_read_char(this->parent()->get_gattc_if(), this->parent()->get_conn_id(), this->propane_level_handle_, ESP_GATT_AUTH_REQ_NONE);
        if (status)
        {
          ESP_LOGW(TAG, "Error sending read request for sensor, status=%d", status);
        }
      }
    }

    bool IGrill::is_same_address_(uint8_t *a, uint8_t *b)
    {
      for (size_t i = 0; i < 6; i++)
      {
        if (a[i] != b[i])
        {
          ESP_LOGD(TAG, "Addresses do not match a: %02X:%02X:%02X:%02X:%02X:%02X b: %02X:%02X:%02X:%02X:%02X:%02X",
                   a[0], a[1], a[2], a[3], a[4], a[5], b[0], b[1], b[2], b[3], b[4], b[5]);
          return false;
        }
      }

      return true;
    }

    void IGrill::dump_config()
    {
      LOG_SENSOR("  ", "Temperature", this->temperature_probe1_sensor_);
      LOG_SENSOR("  ", "Temperature", this->temperature_probe2_sensor_);
      LOG_SENSOR("  ", "Temperature", this->temperature_probe3_sensor_);
      LOG_SENSOR("  ", "Temperature", this->temperature_probe4_sensor_);
      LOG_SENSOR("  ", "Battery Level", this->battery_level_sensor_);
      LOG_SENSOR("  ", "Propane Level", this->propane_level_sensor_);
    }

    IGrill::IGrill()
        : PollingComponent(10000) {}

  } // namespace igrill
} // namespace esphome

#endif // USE_ESP32