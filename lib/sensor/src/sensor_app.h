#pragma once

#include <dosa.h>

#include "sensor_container.h"

#define NO_DEVICE_BLINK_INTERVAL 500  // LED light blink rate when no central is connected
#define PIR_MIN_ACTIVE 1000           // Length of time we require the sensor to be active before calling it a hit
#define PIR_SENSITIVITY_DELAY 5000    // If we get 2 quick hits within this time, still consider it a valid trigger
//#define PIR_POLL 10                 // How often we check the PIR sensor for state change
#define PIR_POLL 1000  // How often we check the PIR sensor for state change
#define PIN_PIR 2      // Data pin for PIR sensor

// Never send a value of 0 through BT (0 will occur on read error)
#define PIR_SENSOR_INACTIVE 1  // PIR state: 'off'
#define PIR_SENSOR_ACTIVE 2    // PIR state: 'on'

namespace dosa::sensor {

class SensorApp final : public dosa::App
{
   public:
    explicit SensorApp(Config const& config) : App(config) {}

    void init() override
    {
        App::init();

        // PIR pin init
        pinMode(PIN_PIR, INPUT);

        // For debug, until FRAM is available
        //setWifi("xxx", "yyy");
    }

    void loop() override
    {
        stdLoop();

        // Check state of the PIR sensor
        if (millis() - pir_last_updated > PIR_POLL) {
            pir_last_updated = millis();

            auto& udp = container.getWiFi().getUdp();

            container.getSerial().write(
                "Dispatch welcome packet to " + Wifi::ipToString(dosa::wifi::sensorBroadcastIp));

            if (udp.beginPacket(dosa::wifi::sensorBroadcastIp, dosa::wifi::sensorBroadcastPort) != 1) {
                container.getSerial().writeln(" error creating packet");
                return;
            }

            String payload = "hello dosa " + String(random(100, 999));
            udp.write(payload.c_str());

            if (udp.endPacket() != 1) {
                container.getSerial().writeln(" error sending packet");
                return;
            }

            container.getSerial().writeln(" done");

            if (container.getWiFi().isConnected()) {
                int packetSize = udp.parsePacket();
                if (packetSize > 0) {
                    container.getSerial().writeln("Packet waiting: " + String(packetSize));
                    container.getSerial().writeln("Data size: " + String(container.getWiFi().getUdp().available()));
                }
            } else {
                container.getSerial().writeln("(Wifi not active)");
            }
        }

        if (false) {
            auto& serial = container.getSerial();
            pir_last_updated = millis();
            byte state = digitalRead(PIN_PIR) == HIGH ? PIR_SENSOR_ACTIVE : PIR_SENSOR_INACTIVE;

            /**
             * Long active state trigger.
             *
             * This is the preferred way to enter a "HIT" state. We time how long the sensor has detected motion and
             * if it exceeds a threshold then we move to active state.
             *
             * NB: the timer for this is activated by the state-change code.
             */
            if (state == PIR_SENSOR_ACTIVE && pir_sensor_value == PIR_SENSOR_ACTIVE &&
                bt_sensor_value == PIR_SENSOR_INACTIVE && (millis() - pir_last_hit > PIR_MIN_ACTIVE)) {
                bt_sensor_value = pir_sensor_value = state;
                serial.writeln("SET: ACTIVE (continuous activity)", dosa::LogLevel::DEBUG);
                // TOOD: wifi broadcast
            }

            /**
             * State change triggers.
             *
             * There are two uses for monitoring PIR sensor state change:
             * 1. Returning to an inactive state
             * 2. Checking for quick successive hits (small motion detected continuously)
             */
            else if (state != pir_sensor_value) {
                pir_sensor_value = state;
                serial.writeln(
                    "sensor: " + String(pir_sensor_value == PIR_SENSOR_ACTIVE ? "active" : "inactive"),
                    dosa::LogLevel::DEBUG);

                if (state == PIR_SENSOR_INACTIVE) {
                    // RETURN TO PASSIVE STATE
                    if (bt_sensor_value != PIR_SENSOR_INACTIVE) {
                        bt_sensor_value = pir_sensor_value;
                        serial.writeln("SET: INACTIVE", dosa::LogLevel::DEBUG);
                        // TOOD: wifi broadcast
                    }
                } else {
                    // TRIGGER ACTIVE STATE FROM SUCCESSIVE HITS
                    if (millis() - pir_last_hit < PIR_SENSITIVITY_DELAY) {
                        bt_sensor_value = pir_sensor_value;
                        serial.writeln("SET: ACTIVE (repeat trigger)", dosa::LogLevel::DEBUG);
                        // TOOD: wifi broadcast
                    }
                    pir_last_hit = millis();
                }
            }
        }
    }

   private:
    SensorContainer container;

    void onWifiConnect() override
    {
        App::onWifiConnect();
        container.getWiFi().getUdp().begin(random(1024, 65536));
    }

    unsigned long pir_last_hit = 0;      // To time how long between quick hits and/or the length of the active state
    unsigned long pir_last_updated = 0;  // Last time we polled the sensor
    byte pir_sensor_value = PIR_SENSOR_INACTIVE;
    byte bt_sensor_value = PIR_SENSOR_INACTIVE;

    Container& getContainer() override
    {
        return container;
    }
};

}  // namespace dosa::sensor
