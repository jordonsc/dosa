#pragma once

#include <dosa.h>

namespace dosa {

enum class LaserResponseCode : uint8_t
{
    SUCCESS,
    FAIL,
    TIMEOUT,
    UNKNOWN,
};

struct LaserMessage
{
    uint8_t const* payload;
    size_t size;
};

namespace {

// Address command success
constexpr uint8_t data_addr_success[4] = {0xFA, 0x04, 0x81, 0x81};
constexpr LaserMessage addr_success{data_addr_success, 4};

// Address command failed
constexpr uint8_t data_addr_failed[5] = {0xFA, 0x84, 0x81, 0x02, 0xFF};
constexpr LaserMessage addr_failed{data_addr_failed, 5};

}  // namespace

class Laser
{
   public:
    Laser()
    {
        Serial1.begin(9600);
    }

    LaserResponseCode setAddress(uint8_t addr)
    {
        uint8_t data[5] = {0xFA, 0x04, 0x01, addr};
        data[4] = getChecksum(data, 4);

        // NB: this has a tendency to fail despite actually succeeding, always update the device_addr
        device_addr = addr;
        return sendCommand({data, 5}, addr_success, addr_failed);
    }

    LaserResponseCode setControlLaser(bool enabled)
    {
        uint8_t enabled_byte = enabled ? 0x01 : 0x00;
        uint8_t data[5] = {device_addr, 0x06, 0x05, enabled_byte};
        data[4] = getChecksum(data, 4);

        uint8_t cmd_success[5] = {device_addr, 0x06, 0x85, 0x01};
        cmd_success[4] = getChecksum(cmd_success, 4);

        uint8_t cmd_fail[5] = {device_addr, 0x06, 0x85, 0x00};
        cmd_fail[4] = getChecksum(cmd_fail, 4);

        return sendCommand({data, 5}, {cmd_success, 5}, {cmd_fail, 5});
    }

    void startContinuousMeasurement()
    {
        uint8_t data[4] = {device_addr, 0x06, 0x03};
        data[3] = getChecksum(data, 3);

        tx(data, 4);
    }

    LaserResponseCode stop()
    {
        uint8_t data[4] = {device_addr, 0x04, 0x02};
        data[3] = getChecksum(data, 3);

        tx(data, 4);
        hardFlush(250);
    }

    /**
     * Checks the laser for new data. Should be run in main loop.
     *
     * Returns true if the distance has been updated.
     */
    bool process()
    {
        static uint8_t pos = 0;
        static uint8_t data[11] = {};

        if (Serial1.available() == 0) {
            return false;
        }

        // Read the header, if it looks like this isn't the header - abort, eventually loop will rsync with the
        // 11-byte pattern.

        while (Serial1.available()) {
            uint8_t b = Serial1.read();
            if (error_correction && b == 0xFF) {
                continue;
            }

            if (pos == 0 && b != device_addr) {
                pos = 0;
                continue;
            } else if (pos == 1 && b != 0x06) {
                pos = 0;
                continue;
            } else if (pos == 2 && b != 0x83) {
                continue;
            }

            data[pos] = b;
            ++pos;

            if (pos == 11) {
                pos = 0;

                if (data[10] != getChecksum(data, 10)) {
                    return false;
                }

                // Device prints ASCII 'ERR' when it can't make a measurement
                if ((data[3] == 'E' && data[4] == 'R' && data[5] == 'R') || (data[6] != '.')) {
                    return false;
                }

                distance = 0;
                distance += data[9] - 0x30;
                distance += (data[8] - 0x30) * 10;
                distance += (data[7] - 0x30) * 100;
                distance += (data[5] - 0x30) * 1000;
                distance += (data[4] - 0x30) * 10000;
                distance += (data[3] - 0x30) * 100000;
                return true;
            }
        }
    }

    [[nodiscard]] uint32_t getDistance() const
    {
        return distance;
    }

    bool getErrorCorrection() const
    {
        return error_correction;
    }
    void setErrorCorrection(bool errorCorrection)
    {
        error_correction = errorCorrection;
    }

   protected:
    void tx(uint8_t const* cmd, size_t sz)
    {
        Serial1.write(cmd, sz);
    }

    size_t rx(uint8_t* buf, size_t s)
    {
        return Serial1.readBytes(buf, s);
    }

    void hardFlush(uint32_t until = 1000)
    {
        auto start = millis();
        while (millis() - start < until) {
            flush(10);
        }
    }

    void flush(uint32_t wait = 50)
    {
        if (wait > 0) {
            waitForData(wait);
        }

        while (Serial1.available()) {
            Serial1.read();
        }
    }

    LaserResponseCode sendCommand(LaserMessage const& msg, LaserMessage const& success, LaserMessage const& fail)
    {
        tx(msg.payload, msg.size);
        if (!waitForData(1000)) {
            return LaserResponseCode::TIMEOUT;
        }

        size_t read = 0, max_read = max(success.size, fail.size);
        uint8_t buffer[max_read];
        memset(buffer, 0, max_read);

        while (read < max_read) {
            buffer[read] = Serial1.read();

            // The schema never uses 0xFF, and occasionally the device will add random 0xFF bytes in the response.
            // We can safely skip over them -
            if (error_correction && buffer[read] == 0xFF) {
                continue;
            }

            ++read;

            if (read == success.size || read == fail.size) {
                if (success.payload != nullptr && memcmp(buffer, success.payload, success.size) == 0) {
                    return LaserResponseCode::SUCCESS;
                } else if (fail.payload != nullptr && memcmp(buffer, fail.payload, fail.size) == 0) {
                    return LaserResponseCode::FAIL;
                }
            }
        }

        return LaserResponseCode::UNKNOWN;
    }

    bool waitForData(uint32_t max_time)
    {
        auto start = millis();
        while (Serial1.available() == 0 && (millis() - start < max_time)) {
            delay(5);
        }
        return Serial1.available() > 0;
    }

    uint8_t getChecksum(uint8_t const* data, size_t len)
    {
        uint8_t checksum = 0;

        for (int i = 0; i < len; ++i) {
            checksum = checksum + data[i];
        }

        return ~checksum + 1;
    }

   private:
    uint32_t distance = 0;
    uint8_t device_addr = 0x80;
    bool error_correction = true;
};

}  // namespace dosa
