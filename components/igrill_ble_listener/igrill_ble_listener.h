#pragma once

#ifdef USE_ESP32

#include "esphome/core/component.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

namespace esphome
{
    namespace igrill_ble_listener
    {

        class IGrillBLEListener : public esp32_ble_tracker::ESPBTDeviceListener
        {
        protected:
            bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;
            void on_scan_end() override;
            std::vector<uint64_t> already_discovered_;
        };

    } // namespace igrill_ble_listener
} // namespace esphome

#endif
