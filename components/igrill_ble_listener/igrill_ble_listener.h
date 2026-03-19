#pragma once
#ifdef USE_ESP32

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

namespace esphome
{
  namespace igrill_ble_listener
  {
    class IGrillBLEListener : public Component, public esp32_ble_tracker::ESPBTDeviceListener
    {
    public:
      void setup() override {}
      void dump_config() override {}
      float get_setup_priority() const override { return setup_priority::DATA; }
    protected:
      bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;
      void on_scan_end() override;
      std::vector<uint64_t> already_discovered_;
    };
  } // namespace igrill_ble_listener
} // namespace esphome

#endif // USE_ESP32
