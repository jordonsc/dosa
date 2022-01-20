#pragma once

#include <Arduino.h>

#define DOSA_SETTINGS_HEADER "DS10"
#define DOSA_SETTINGS_OVERSIZE_READ "#ERR-OVERSIZE"
#define DOSA_SETTINGS_DEFAULT_PIN "dosa"

namespace dosa {

/**
 * Structure of settings:
 *   Size   Type      Detail
 *   ----------------------------------
 *   4      char      Header
 *   2      uint16    Size of pin
 *   ?      char      Pin
 *   2      uint16    Size of device name
 *   ?      char      Device name
 *   2      uint16    Size of wifi SSID
 *   ?      char      Wifi SSID
 *   2      uint16    Size of wifi password
 *   ?      char      Wifi password
 */
class Settings : public Loggable
{
   public:
    explicit Settings(Fram& ram, SerialComms* serial = nullptr) : Loggable(serial), ram(ram) {}

    /**
     * Load values from FRAM.
     *
     * If the header is a mismatch, or any data appears corrupt, default values will be loaded for everything.
     * Returns true if settings were loaded clean, false if there are issues and defaults were used.
     */
    bool load()
    {
        if (ram.readHeader() != DOSA_SETTINGS_HEADER) {
            // Settings missing or out-of-date, use default values
            logln("FRAM header incorrect, using default settings", dosa::LogLevel::WARNING);
            setDefaults();
            return false;
        }

        uint32_t ptr = 4;  // Size of header

        auto read_block = [this, &ptr](String& s) -> bool {
            s = readVarChar(ptr);
            if (s == DOSA_SETTINGS_OVERSIZE_READ) {
                setDefaults();
                return false;
            }
            ptr += s.length() + 2;
            return true;
        };

        if (!read_block(pin))
            return false;

        if (!read_block(device_name))
            return false;

        if (!read_block(wifi_ssid))
            return false;

        if (!read_block(wifi_password))
            return false;

        // Validate values
        bool valid = true;

        if (pin == "") {
            // Not allowed a blank pin
            pin = "dosa";
            valid = false;
        }

        if (device_name == "") {
            // Not allowed a blank device name
            device_name = "DOSA " + String(random(1000, 9999));
            valid = false;
        }

        updateDeviceNameBytes();

        return valid;
    }

    /**
     * Write currently set values to FRAM.
     */
    void save()
    {
        // header size (4) + 2x4 size markers + value lengths
        uint32_t size = 12 + pin.length() + device_name.length() + wifi_ssid.length() + wifi_password.length();
        uint8_t payload[size];
        uint8_t* ptr = payload;

        memcpy(ptr, DOSA_SETTINGS_HEADER, 4);
        ptr += 4;

        auto write_block = [&ptr](String const& s) {
            uint16_t item_size = s.length();
            memcpy(ptr, &item_size, 2);
            memcpy(ptr + 2, s.c_str(), s.length());
            ptr += item_size + 2;
        };

        write_block(pin);
        write_block(device_name);
        write_block(wifi_ssid);
        write_block(wifi_password);

        ram.write(0, payload, size);
        logln("Settings written to FRAM", dosa::LogLevel::INFO);
    }

    void setDefaults()
    {
        pin = DOSA_SETTINGS_DEFAULT_PIN;
        device_name = "DOSA " + String(random(1000, 9999));
        wifi_ssid = "";
        wifi_password = "";
        updateDeviceNameBytes();
    }

    [[nodiscard]] String const& getDeviceName() const
    {
        return device_name;
    }

    [[nodiscard]] char const* getDeviceNameBytes() const
    {
        return device_name_bytes;
    }

    bool setDeviceName(String const& deviceName)
    {
        if (deviceName.length() < 2 || deviceName.length() > 20) {
            return false;
        }

        device_name = deviceName;
        updateDeviceNameBytes();

        return true;
    }

    [[nodiscard]] String const& getPin() const
    {
        return pin;
    }

    bool setPin(String const& newPin)
    {
        if (newPin.length() < 4) {
            return false;
        }

        pin = newPin;

        return true;
    }

    [[nodiscard]] String const& getWifiSsid() const
    {
        return wifi_ssid;
    }

    void setWifiSsid(String const& wifiSsid)
    {
        wifi_ssid = wifiSsid;
    }

    [[nodiscard]] String const& getWifiPassword() const
    {
        return wifi_password;
    }

    void setWifiPassword(String const& wifiPassword)
    {
        wifi_password = wifiPassword;
    }

   protected:
    Fram& ram;
    String device_name;
    char device_name_bytes[20] = {0};
    String pin;
    String wifi_ssid;
    String wifi_password;

    /**
     * Rebuild the 20x char array for the device name.
     */
    void updateDeviceNameBytes()
    {
        memset(device_name_bytes, 0, 20);
        memcpy(device_name_bytes, device_name.c_str(), device_name.length());
    }

    /**
     * Read a 16-bit size and then that many bytes into a string.
     *
     * Advance your pointer 2 + returned string length.
     */
    String readVarChar(uint32_t addr)
    {
        uint16_t size;
        ram.read(addr, (uint8_t*)&size, 2);

        if (size == 0) {
            return "";
        } else if (size > 500) {
            return DOSA_SETTINGS_OVERSIZE_READ;
        } else {
            char buffer[size + 1];
            ram.read(addr + 2, (uint8_t*)buffer, size);
            buffer[size] = 0;
            return String(buffer);
        }
    }
};

}  // namespace dosa
