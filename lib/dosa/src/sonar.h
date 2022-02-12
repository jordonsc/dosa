#pragma once

namespace dosa {

class Sonar
{
   public:
    Sonar()
    {
        Serial1.begin(9600);
    }

    /**
     * Checks the sonar for new data. Should be run in main loop.
     *
     * Returns true if the distance has been updated.
     */
    bool process()
    {
        if (Serial1.available()) {
            uint8_t c = Serial1.read();

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

   private:
    uint16_t distance = 0;
    uint8_t buffer[4] = {0};  // serial read buffer
    uint8_t idx = 0;          // serial read index
};

}  // namespace dosa
