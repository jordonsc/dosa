#pragma once

#include <Uart.h>
#include <wiring_private.h>

#define DOSA_SONAR_SERIAL &sercom0
#define DOSA_SONAR_RX_PIN 5
#define DOSA_SONAR_TX_PIN 6
#define DOSA_SONAR_RX_PAD SERCOM_RX_PAD_1
#define DOSA_SONAR_TX_PAD UART_TX_PAD_0

namespace dosa {

class Sonar
{
   public:
    Sonar()
        : sonar_serial(DOSA_SONAR_SERIAL, DOSA_SONAR_RX_PIN, DOSA_SONAR_TX_PIN, DOSA_SONAR_RX_PAD, DOSA_SONAR_TX_PAD)
    {
        pinPeripheral(DOSA_SONAR_RX_PIN, PIO_SERCOM_ALT);
        pinPeripheral(DOSA_SONAR_TX_PIN, PIO_SERCOM_ALT);

        sonar_serial.begin(9600);

        instance = this;
    }

    /**
     * Checks the sonar for new data. Should be run in main loop.
     *
     * Returns true if the distance has been updated.
     */
    bool process()
    {
        if (sonar_serial.available()) {
            uint8_t c = sonar_serial.read();

            if (idx == 0 && c == 0xFF) {
                // Header byte should be 0xFF
                buffer[idx++] = c;
            } else if ((idx == 1) || (idx == 2)) {
                // Distance found in middle byte
                buffer[idx++] = c;
            } else if (idx == 3) {
                // Checksum byte
                idx = 0;
                uint8_t sum = buffer[0] + buffer[1] + buffer[2];
                if (sum == c) {
                    distance = ((uint16_t)buffer[1] << 8) | buffer[2];
                    return true;
                }
            }
        }

        return false;
    }

    [[nodiscard]] uint16_t getDistance() const
    {
        return distance;
    }

    [[nodiscard]] static Sonar* getInstance()
    {
        return instance;
    }

    void serialInterrupt()
    {
        sonar_serial.IrqHandler();
    }

   private:
    Uart sonar_serial;
    uint16_t distance = 0;
    uint8_t buffer[4] = {0};  // serial read buffer
    uint8_t idx = 0;          // serial read index

    static Sonar* instance;
};

Sonar* Sonar::instance = nullptr;

}  // namespace dosa

void SERCOM0_Handler()
{
    auto* instance = dosa::Sonar::getInstance();
    if (instance != nullptr) {
        instance->serialInterrupt();
    }
}
