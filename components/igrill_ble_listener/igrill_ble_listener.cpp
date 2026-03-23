#include "igrill_ble_listener.h"

#ifdef USE_ESP32

namespace esphome {
namespace igrill_ble_listener {

static const char *const TAG = "igrill_ble_listener";

// Known iGrill / Weber / iDevices service UUIDs used for device identification
static const std::string IGRILL_MINI_SERVICE = "63c70000-4a82-4261-95ff-92cf32477861";
static const std::string IGRILL_MINIV2_SERVICE = "9d610c43-ae1d-41a9-9b09-3c7ecd5c6035";
static const std::string IGRILLV2_SERVICE = "a5c50000-f186-4bd6-97f2-7ebacba0d708";
static const std::string IGRILLV202_SERVICE = "ada7590f-2e6d-469e-8f7b-1822b386a5e9";
static const std::string IGRILLV3_SERVICE = "6e910000-58dc-41c7-943f-518b278cea88";
static const std::string IDEVICES_SERVICE = "19450000-9b05-40bb-80d8-7c85840aec34";
static const std::string PULSE_1000_SERVICE = "7e920000-68dc-41c7-943f-518b278cea87";
static const std::string PULSE_2000_SERVICE = "7e920000-68dc-41c7-943f-518b278cea88";

bool IGrillBLEListener::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  for (auto &uuid : device.get_service_uuids()) {
    char uuid_buf[37];
    std::string uuid_str = uuid.to_str(uuid_buf);
    // Lowercase for comparison
    std::transform(uuid_str.begin(), uuid_str.end(), uuid_str.begin(), ::tolower);

    bool is_igrill = (uuid_str == IGRILL_MINI_SERVICE || uuid_str == IGRILL_MINIV2_SERVICE ||
                      uuid_str == IGRILLV2_SERVICE || uuid_str == IGRILLV202_SERVICE || uuid_str == IGRILLV3_SERVICE ||
                      uuid_str == IDEVICES_SERVICE || uuid_str == PULSE_1000_SERVICE || uuid_str == PULSE_2000_SERVICE);

    if (is_igrill) {
      uint64_t address = device.address_uint64();
      // Only log each device once per scan cycle
      if (std::find(already_discovered_.begin(), already_discovered_.end(), address) == already_discovered_.end()) {
        already_discovered_.push_back(address);
        ESP_LOGI(TAG, "Found iGrill device, MAC address: %s  RSSI: %d dBm", device.address_str().c_str(),
                 device.get_rssi());
      }
      return true;
    }
  }
  return false;
}

void IGrillBLEListener::on_scan_end() { already_discovered_.clear(); }

}  // namespace igrill_ble_listener
}  // namespace esphome

#endif  // USE_ESP32
