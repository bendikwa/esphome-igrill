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
          ESP_LOGV(TAG, "This ESP_GATTC_CONNECT_EVT is not for me. (remote_bda mismatch");
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
          ESP_LOGV(TAG, "This ESP_GATTC_SEARCH_CMPL_EVT is not for me. (conn_id mismatch");
          break;
        }
        // Detect IGrill model and get hadles for the appropriate number of probes.
        IGrill::detect_and_init_igrill_model_();

        // Get handle for battery level
        this->battery_level_handle_ = get_handle_(BATTERY_SERVICE_UUID, BATTERY_LEVEL_UUID);
        this->value_readers_[this->battery_level_handle_] = &IGrill::read_battery_;

        // Get handle for propane level, if configured
        if (this->propane_level_sensor_ != nullptr)
        {
          this->propane_level_handle_ = get_handle_(PROPANE_LEVEL_SERVICE_UUID, PROPANE_LEVEL);
          this->value_readers_[this->propane_level_handle_] = &IGrill::read_propane_;
        }

        // Get handles for authentication
        this->app_challenge_handle_ = get_handle_(AUTHENTICATION_SERVICE_UUID, APP_CHALLENGE_UUID);
        this->device_response_handle_ = get_handle_(AUTHENTICATION_SERVICE_UUID, DEVICE_RESPONSE_UUID);
        this->device_challenge_handle_ = get_handle_(AUTHENTICATION_SERVICE_UUID, DEVICE_CHALLENGE_UUID);
        this->value_readers_[this->device_challenge_handle_] = &IGrill::loopback_device_challenge_response_;

        ESP_LOGD(TAG, "Starting autheintication process");
        send_authentication_challenge_();

        this->node_state = esp32_ble_tracker::ClientState::ESTABLISHED;
        break;
      }

      case ESP_GATTC_WRITE_CHAR_EVT:
      {
        if (param->write.conn_id != this->parent()->get_conn_id())
        {
          ESP_LOGV(TAG, "This ESP_GATTC_WRITE_CHAR_EVT is not for me. (conn_id mismatch");
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
          ESP_LOGV(TAG, "This ESP_GATTC_READ_CHAR_EVT is not for me. (conn_id mismatch");
        }
        else if (param->read.status != ESP_GATT_OK)
        {
          ESP_LOGW(TAG, "Error reading char at handle 0x%x, status=%d", param->read.handle, param->read.status);
        }
        else
        {
          if (value_readers_.count(param->read.handle))
          {
            ESP_LOGV(TAG, "Read char event received for handle: 0x%x", param->read.handle);
            (this->*value_readers_[param->read.handle])(param->read.value, param->read.value_len);
          }
          else{
            ESP_LOGD(TAG, "No read function defined for handle: 0x%x", param->read.handle);
          }
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
      else if (has_service_(IDEVICES_KITCHEN_TEMPERATURE_SERVICE_UUID))
      {
        ESP_LOGI(TAG, "Detected model: iDevices Kitchen");
        num_probes = 2;
        service = IDEVICES_KITCHEN_TEMPERATURE_SERVICE_UUID;
      }
      else if (has_service_(PULSE_1000_TEMPERATURE_SERVICE_UUID))
      {
        ESP_LOGI(TAG, "Detected model: Pulse 1000");
        num_probes = 2;
        service = PULSE_1000_TEMPERATURE_SERVICE_UUID;
      }
      else if (has_service_(PULSE_2000_TEMPERATURE_SERVICE_UUID))
      {
        ESP_LOGI(TAG, "Detected model: Pulse 2000");
        num_probes = 4;
        service = PULSE_2000_TEMPERATURE_SERVICE_UUID;
      }
      else
      {
        ESP_LOGE(TAG, "Could not identify IGrill model");
        return;
      }
      IGrill::add_temperature_probe_handles_(service);
      this->temperature_unit_handle_ = get_handle_(service, TEMPERATURE_UNIT_UUID);
      this->value_readers_[this->temperature_unit_handle_] = &IGrill::read_temperature_unit_;
    }

    bool IGrill::has_service_(const char *service)
    {
      auto *srv = this->parent()->get_service(esp32_ble_tracker::ESPBTUUID::from_raw(service));
      if (srv == nullptr)
      {
        ESP_LOGV(TAG, "No service found at service %s", service);
        return false;
      }
      return true;
    }

    void IGrill::add_temperature_probe_handles_(const char *service)
    {
      std::vector<const char *> probes = {PROBE1_TEMPERATURE, PROBE2_TEMPERATURE, PROBE3_TEMPERATURE, PROBE4_TEMPERATURE};
      std::vector<void (esphome::igrill::IGrill::*)(uint8_t *, uint16_t)> read_functions = {&IGrill::read_temperature1_, &IGrill::read_temperature2_, &IGrill::read_temperature3_, &IGrill::read_temperature4_};
      for (int i = 0; i < this->num_probes; i++)
      {
        if (this->sensors_[i]) // only add handles for configured sensors
        {
          uint16_t probe_handle = get_handle_(service, probes[i]);
          this->probe_handles_.push_back(probe_handle);
          this->value_readers_[probe_handle] = read_functions[i];
          ESP_LOGV(TAG, "Probe nuber %d added with handle 0x%x", i, probe_handle);
        }
        else
        {
          ESP_LOGD(TAG, "No sensor configured for probe nuber %d. Skipping", i+1);
        }
      }
      if (service == PULSE_1000_TEMPERATURE_SERVICE_UUID || service == PULSE_2000_TEMPERATURE_SERVICE_UUID)
      {
        // Add heating element probes for the Pulse grill if any are configured
        if (pulse_heating_actual1_ || pulse_heating_actual2_ || pulse_heating_setpoint1_ || pulse_heating_setpoint2_)
        {
          this->pulse_element_handle_ = get_handle_(PULSE_ELEMENT_SERVICE_UUID, PULSE_ELEMENT_UUID);
          this->value_readers_[this->pulse_element_handle_] = &IGrill::read_pulse_element_;
        }
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

    void IGrill::read_pulse_element_(uint8_t *raw_value, uint16_t value_len)
    {
      if (this->pulse_heating_actual1_){
        std::string actual1;
        actual1.assign(reinterpret_cast<char*>(raw_value)+1, 3);
        ESP_LOGV(TAG, "Parsed actual1 form pulse element data %s", actual1);
        this->pulse_heating_actual1_->publish_state(stoi(actual1));
      }
      if (this->pulse_heating_actual2_){
        std::string actual2;
        actual2.assign(reinterpret_cast<char*>(raw_value)+5, 3);
        ESP_LOGV(TAG, "Parsed actual2 form pulse element data %s", actual2);
        this->pulse_heating_actual2_->publish_state(stoi(actual2));
      }
      if (this->pulse_heating_setpoint1_){
        std::string setpoint1_;
        setpoint1_.assign(reinterpret_cast<char*>(raw_value)+9, 3);
        ESP_LOGV(TAG, "Parsed setpoint1 form pulse element data %s", setpoint1_);
        this->pulse_heating_setpoint1_->publish_state(stoi(setpoint1_));
      }
      if (this->pulse_heating_setpoint2_){
        std::string setpoint2_;
        setpoint2_.assign(reinterpret_cast<char*>(raw_value)+13, 3);
        ESP_LOGV(TAG, "Parsed setpoint2 form pulse element data %s", setpoint2_);
        this->pulse_heating_setpoint2_->publish_state(stoi(setpoint2_));
      }
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
      ESP_LOGI(TAG, "Setting temperature unit based on device: %s", this->unit_of_measurement_);
      for (auto & sensor : this->sensors_)
      {
        if (sensor)
        {
          sensor->set_unit_of_measurement(this->unit_of_measurement_);
        }
      }
      request_read_values_();
    }
    
    void IGrill::read_temperature_(uint8_t *raw_value, uint16_t value_len, int probe)
    {
      uint16_t raw_temp = (raw_value[1] << 8) | raw_value[0];
      ESP_LOGV(TAG, "Parsing temperature from probe %d", probe);
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
        this->sensors_[probe]->publish_state(temp);
      }
    }

    void IGrill::update()
    {
      switch (this->node_state)
      {
      case esp32_ble_tracker::ClientState::ESTABLISHED:
        if (!this->unit_of_measurement_)
        {
          ESP_LOGD(TAG, "Requesting read of temperature unit");
          request_temp_unit_read_();
        }
        else
        {
          ESP_LOGD(TAG, "Requesting read of all values");
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
      ESP_LOGV(TAG, "Requesting read of encrypted device response from device on handle (0x%x)", this->device_challenge_handle_);
      auto status = esp_ble_gattc_read_char(this->parent()->get_gattc_if(), this->parent()->get_conn_id(), this->device_challenge_handle_, ESP_GATT_AUTH_REQ_NONE);
      if (status)
      {
        ESP_LOGW(TAG, "Error sending read request for device_challenge, status=%d, handle=0x%x", status, this->device_challenge_handle_);
      }
    }

    void IGrill::request_temp_unit_read_()
    {
      ESP_LOGV(TAG, "Requesting read of temperature unit on handle (0x%x)", this->temperature_unit_handle_);
      auto status = esp_ble_gattc_read_char(this->parent()->get_gattc_if(), this->parent()->get_conn_id(), this->temperature_unit_handle_, ESP_GATT_AUTH_REQ_NONE);
      if (status)
      {
        ESP_LOGW(TAG, "Error sending read request for temperature unit, status=%d, handle=0x%x", status, this->temperature_unit_handle_);
      }
    }

    void IGrill::request_read_values_()
    {
      esp_err_t status = ESP_OK;
      // Read battery level
      if (this->battery_level_sensor_ != nullptr)
      {
        ESP_LOGV(TAG, "Requesting read of battery level on handle (0x%x)", this->battery_level_handle_);
        status = esp_ble_gattc_read_char(this->parent()->get_gattc_if(), this->parent()->get_conn_id(), this->battery_level_handle_, ESP_GATT_AUTH_REQ_NONE);
        if (status)
        {
          ESP_LOGW(TAG, "Error sending read request for sensor, status=%d, handle=0x%x", status, this->battery_level_handle_);
        }
      }

      // Read temperature probes
      for (auto & probe_handle : probe_handles_)
      {
        ESP_LOGV(TAG, "Requesting read of temperature probe on handle (0x%x)", probe_handle);
        status = esp_ble_gattc_read_char(this->parent()->get_gattc_if(), this->parent()->get_conn_id(), probe_handle, ESP_GATT_AUTH_REQ_NONE);
        if (status)
        {
          ESP_LOGW(TAG, "Error sending read request for sensor, status=%d, handle=0x%x", status, probe_handle);
        }
      }

      // Read propane level
      if (this->propane_level_sensor_ != nullptr)
      {
        ESP_LOGV(TAG, "Requesting read of propane level on handle (0x%x)", this->propane_level_handle_);
        status = esp_ble_gattc_read_char(this->parent()->get_gattc_if(), this->parent()->get_conn_id(), this->propane_level_handle_, ESP_GATT_AUTH_REQ_NONE);
        if (status)
        {
          ESP_LOGW(TAG, "Error sending read request for sensor, status=%d, handle=0x%x", status, this->propane_level_handle_);
        }
      }

      // Read pulse element
      if (pulse_heating_actual1_ || pulse_heating_actual2_ || pulse_heating_setpoint1_ || pulse_heating_setpoint2_)
      {
        ESP_LOGV(TAG, "Requesting read of pulse element on handle (0x%x)", this->pulse_element_handle_);
        status = esp_ble_gattc_read_char(this->parent()->get_gattc_if(), this->parent()->get_conn_id(), this->pulse_element_handle_, ESP_GATT_AUTH_REQ_NONE);
        if (status)
        {
          ESP_LOGW(TAG, "Error sending read request for sensor, status=%d, handle=0x%x", status, this->pulse_element_handle_);
        }
      }
    }

    bool IGrill::is_same_address_(uint8_t *a, uint8_t *b)
    {
      for (size_t i = 0; i < 6; i++)
      {
        if (a[i] != b[i])
        {
          ESP_LOGV(TAG, "Addresses do not match a: %02X:%02X:%02X:%02X:%02X:%02X b: %02X:%02X:%02X:%02X:%02X:%02X",
                   a[0], a[1], a[2], a[3], a[4], a[5], b[0], b[1], b[2], b[3], b[4], b[5]);
          return false;
        }
      }

      return true;
    }

    void IGrill::dump_config()
    {
      for (auto & element : this->sensors_)
      {
        if (element)
        {
          LOG_SENSOR("  ", "Temperature", element);
        }
      }
      if (this->battery_level_sensor_ != nullptr)
      {
        LOG_SENSOR("  ", "Battery Level", this->battery_level_sensor_);
      }
      if (this->propane_level_sensor_ != nullptr)
      {
        LOG_SENSOR("  ", "Propane Level", this->propane_level_sensor_);
      }
      if (this->pulse_heating_actual1_ != nullptr)
      {
        LOG_SENSOR("  ", "Pulse heating actual 1", this->pulse_heating_actual1_);
      }
      if (this->pulse_heating_actual2_ != nullptr)
      {
        LOG_SENSOR("  ", "Pulse heating actual 2", this->pulse_heating_actual2_);
      }
      if (this->pulse_heating_setpoint1_ != nullptr)
      {
        LOG_SENSOR("  ", "Pulse heating setpoint 1", this->pulse_heating_setpoint1_);
      }
      if (this->pulse_heating_setpoint2_ != nullptr)
      {
        LOG_SENSOR("  ", "Pulse heating setpoint 2", this->pulse_heating_setpoint2_);
      }
    }

    IGrill::IGrill()
        : PollingComponent(10000) {}

  } // namespace igrill
} // namespace esphome

#endif // USE_ESP32
