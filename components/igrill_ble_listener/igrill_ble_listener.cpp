#include "igrill_ble_listener.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

namespace esphome
{
    namespace igrill_ble_listener
    {

        static const char *const TAG = "igrill_ble_listener";

        bool IGrillBLEListener::parse_device(const esp32_ble_tracker::ESPBTDevice &device)
        {

            auto address_ = device.address();
            if (address_[0] == 0x70 && address_[1] == 0x91 && address_[2] == 0x8F)
            {
                const uint64_t address = device.address_uint64();
                for (auto &disc : this->already_discovered_)
                {
                    if (disc == address)
                    {
                        return true;
                    }
                }
                this->already_discovered_.push_back(address);

                ESP_LOGI(TAG, "Found IGrill device Name: %s (MAC: %s)", device.get_name().c_str(), device.address_str().c_str());
                return true;
            }
            return false;
        }

        void IGrillBLEListener::on_scan_end()
        {
            this->already_discovered_.clear();
        }

    } // namespace igrill_ble_listener
} // namespace esphome

#endif
