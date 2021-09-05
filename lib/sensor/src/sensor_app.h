#pragma once

#include <dosa.h>

#define NO_DEVICE_BLINK_INTERVAL 500
#define LOW_PWR_TIMER 300000  // Time before we turn off all LEDs to save power (ms)
#define PIR_POLL 50           // How often we check the PIR sensor for state change
#define PIN_PIR 2             // Data pin for PIR sensor

namespace dosa::sensor {

class SensorApp final : public dosa::App
{
   public:
    explicit SensorApp(Config const& config)
        : App(config, dosa::bt::svc_sensor),
          bt_char_pir(dosa::bt::char_pir, BLERead | BLENotify)
    {
        bt_service.addCharacteristic(bt_char_pir);
    }

    void init() override
    {
        App::init();

        // BT PIR characteristic
        bt_char_pir.writeValue(1);

        // PIR pin init
        pinMode(PIN_PIR, INPUT);
    }

    void loop() override
    {
        auto& serial = container.getSerial();
        auto& bt = container.getBluetooth();
        auto& pool = container.getDevicePool();
        auto& lights = container.getLights();

        checkCentral();

        // Check state of the PIR sensor
        if (millis() - pir_last_updated > PIR_POLL) {
            pir_last_updated = millis();

            byte state = digitalRead(PIN_PIR) + 1;  // + 1 because we never want to send 0 as the value

            if (state != pir_sensor_value) {
                pir_sensor_value = state;
                serial.writeln("Update PIR sensor value: " + String(pir_sensor_value), dosa::LogLevel::DEBUG);
                bt_char_pir.writeValue(pir_sensor_value);
            }
        }
    }

   private:
    Container container;
    BLEByteCharacteristic bt_char_pir;

    unsigned long start_time = 0;
    unsigned long pir_last_updated = 0;
    byte pir_sensor_value = 1;

    Container& getContainer() override
    {
        return container;
    }
};

}  // namespace dosa::sensor
