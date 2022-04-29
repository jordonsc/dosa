#pragma once

#include <Arduino.h>
#include <dosa_comms.h>

#include "const.h"

#define DOSA_SETTINGS_HEADER "DS23"

/**
 * Default device Bluetooth password.
 */
constexpr static char const* default_pin = "dosa";

/**
 * Minimum number of pixels that are considered 'changed' before we accept a trigger. Increase this to eliminate
 * single-pixel or edge anomalies.
 */
constexpr static uint8_t default_pir_min_pixels = 3;

/**
 * Temp change (in Celsius) before considering any single pixel as "changed". This is a de-noising threshold, increase
 * this number to reduce the amount of noise the algorithm is sensitive to.
 */
constexpr static float default_pir_pixel_delta = 1.5;

/**
 * The total temperature delta across all pixels before firing a trigger. This is the primary sensitivity metric, it
 * is also filtered against noise by SENSOR_SINGLE_DELTA_THRESHOLD so it won't show a true full-grid delta.
 */
constexpr static float default_pir_total_delta = 25.0;

/**
 * Distance in mm the sonar should be <= when halting the door open sequence. The sonar should be reading the door's
 * distance from its apex/threshold.
 */
constexpr static uint16_t default_door_open_distance = 500;

/**
 * Time in milliseconds the door spends in then open-wait status, holding in an open position before closing again.
 */
constexpr static uint32_t default_door_open_wait = 3000;

/**
 * Time in milliseconds we wait before allowing further action after a trigger sequence.
 */
constexpr static uint32_t default_door_cool_down = 3000;

/**
 * Fixed number of ticks we close the door for. This will translate to an approximate distance, it should be a small
 * amount greater than required.
 */
constexpr static uint32_t default_door_close_ticks = 15000;

/**
 * Number of consecutive reads with a reduced distance before firing the trigger.
 *
 * Increase to reduce noise.
 */
constexpr static uint16_t default_range_trigger_threshold = 3;

/**
 * Percentage of previous distance that's considered a trigger.
 */
constexpr static float default_range_trigger_coefficient = 0.9;

/**
 * Fixed distance for the sonar resting state. Set to zero for automatic detection.
 *
 * Recommended you set this value for outdoor devices, or devices aiming at non-perpendicular or non-solid surfaces.
 */
constexpr static uint16_t default_range_fixed_calibration = 0;

/**
 * Time the relay is active once triggered. If set to 0, the relay will be a toggle.
 */
constexpr static uint32_t default_relay_activation_time = 5000;

#define DOSA_SETTINGS_OVERSIZE_READ "#ERR-OVERSIZE"
constexpr static char const* current_settings_header = DOSA_SETTINGS_HEADER;
constexpr static char const* null_str = "";
constexpr static uint8_t zero_8 = 0;
constexpr static uint16_t zero_16 = 0;

namespace dosa {

/**
 * Indices 0-99 are reserved for global settings.
 * Indices 100-199 are available for application-specific settings.
 * Indices 200-255 are reserved for future use.
 */
enum class CfgItem : uint8_t
{
    DEVICE_NAME = 0,
    PASSWORD = 1,
    LOCK_STATE = 2,
    WIFI_SSID = 3,
    WIFI_PASSWORD = 4,
    STATS_SVR_ADDR = 5,
    STATS_SVR_PORT = 6,
};


/**
 * Structure of settings:
 *   Size   Type      Detail
 *   ----------------------------------
 *   1      char      Header - validates we've got a DOSA settings stored (and correct version)
 *   1      uint8     Device lock state
 *   2      uint16    Size of device password
 *   ?      char      Device password (aka pin)
 *   2      uint16    Size of device name
 *   ?      char      Device name
 *   2      uint16    Size of wifi SSID
 *   ?      char      Wifi SSID
 *   2      uint16    Size of wifi password
 *   ?      char      Wifi password
 *   2      uint16    Size of log server address
 *   ?      char      Log server address
 *   2      uint16    Log server port
 *   1      uint8     PIR cfg: PIR_MIN_PIXELS_THRESHOLD
 *   4      float     PIR cfg: PIR_SINGLE_DELTA_THRESHOLD
 *   4      float     PIR cfg: PIR_TOTAL_DELTA_THRESHOLD
 *   2      uint16    Door cfg: DOOR_OPEN_DISTANCE
 *   4      uint32    Door cfg: DOOR_OPEN_WAIT_TIME
 *   4      uint32    Door cfg: DOOR_COOL_DOWN
 *   4      uint32    Door cfg: DOOR_CLOSE_TICKS
 *   2      uint16    Range cfg: RANGE_TRIGGER_THRESHOLD
 *   2      uint16    Range cfg: RANGE_FIXED_CALIBRATION
 *   4      float     Range cfg: RANGE_TRIGGER_COEFFICIENT
 *   4      float     Relay cfg: RELAY_ACTIVATION_TIME
 */
class Settings : public Loggable
{
   public:
    explicit Settings(Fram& ram, SerialComms* serial = nullptr) : Loggable(serial), ram(ram)
    {
        // The floating-point values for sensor config are expected to be 32-bit
        static_assert(sizeof(float) == 4, "Float size is not 4");
    }

    /**
     * Load values from FRAM.
     *
     * If the header is a mismatch, or any data appears corrupt, default values will be loaded for everything.
     * Returns true if settings were loaded clean, false if there are issues and defaults were used.
     *
     * If re_init is true, the FRAM will be re-initialised first. See reInitRam().
     */
    bool load(bool re_init = true)
    {
        if (re_init) {
            reInitRam();
        }

        String currentSettingsVersion = ram.readHeader();
        auto c_ver = getSettingsVersion(currentSettingsVersion);
        bool upgrading = false;

        if (currentSettingsVersion != current_settings_header) {
            // Settings missing or out-of-date
            if (canUpgrade(currentSettingsVersion)) {
                upgrading = true;
                logln("Upgrading settings from previous build", dosa::LogLevel::WARNING);
            } else {
                logln("FRAM header incorrect, using default settings", dosa::LogLevel::WARNING);
                setDefaults();
                return false;
            }
        }

        uint32_t ptr = 4;  // Size of header

        auto read_block = [this, &ptr, c_ver](String& s, uint8_t req_ver = 0, char const* src = nullptr) -> bool {
            if (req_ver > 0 && c_ver < req_ver) {
                s = String(src);
                return true;
            }

            s = readVarChar(ptr);
            if (s == DOSA_SETTINGS_OVERSIZE_READ) {
                logln("Oversize read warning", LogLevel::ERROR);
                return false;
            }
            ptr += s.length() + 2;
            return true;
        };

        auto read_var = [this, &ptr, c_ver](void* s, size_t size, uint8_t req_ver = 0, void* src = nullptr) -> void {
            if (req_ver > 0 && c_ver < req_ver) {
                memcpy(s, src, size);
                return;
            }

            ram.read(ptr, (uint8_t*)s, size);
            ptr += size;
        };

        read_var(&locked, 1, 20, (void*)(&zero_8));

        if (!read_block(pin)) {
            logln("Bad read: PIN", LogLevel::ERROR);
            setDefaults();
            return false;
        }

        if (!read_block(device_name)) {
            logln("Bad read: device name", LogLevel::ERROR);
            setDefaults();
            return false;
        }

        if (!read_block(wifi_ssid)) {
            logln("Bad read: wifi SSID", LogLevel::ERROR);
            setDefaults();
            return false;
        }

        if (!read_block(wifi_password)) {
            logln("Bad read: wifi password", LogLevel::ERROR);
            setDefaults();
            return false;
        }

        if (!read_block(stats_server_addr, 23, null_str)) {
            logln("Bad read: stats server", LogLevel::ERROR);
            setDefaults();
            return false;
        }
        read_var(&stats_server_port, 2, 23, (void*)(&zero_16));

        read_var(&pir_min_pixels, 1);
        read_var(&pir_pixel_delta, 4);
        read_var(&pir_total_delta, 4);

        read_var(&door_open_distance, 2);
        read_var(&door_open_wait, 4);
        read_var(&door_cool_down, 4);
        read_var(&door_close_ticks, 4);

        read_var(&range_trigger_threshold, 2);
        read_var(&range_fixed_calibration, 2, 18, (void*)(&default_range_fixed_calibration));
        read_var(&range_trigger_coefficient, 4, 19, (void*)(&default_range_trigger_coefficient));

        read_var(&relay_activation_time, 4, 22, (void*)(&default_relay_activation_time));

        if (!read_block(listen_devices, 21, null_str)) {
            logln("Bad read: listen devices", LogLevel::ERROR);
            setDefaults();
            return false;
        }

        // Validate values
        bool valid = true;

        if (pin == "") {
            // Not allowed a blank pin
            pin = default_pin;
            logln("Device PIN invalid, resetting to default", LogLevel::ERROR);
            valid = false;
        }

        if (device_name == "") {
            // Device name cannot be blank
            device_name = "DOSA " + String(random(1000, 9999));
            logln("Device name invalid, creating new name", LogLevel::ERROR);
            valid = false;
        }

        updateDeviceNameBytes();

        // A save operation should be performed if validation failed, or we performed an upgrade
        return valid && !upgrading;
    }

    /**
     * Write currently set values to FRAM.
     *
     * If re_init is true, the FRAM will be re-initialised first. See reInitRam().
     */
    void save(bool re_init = true)
    {
        if (re_init) {
            reInitRam();
        }

        /**
         * This is a safeguard to ensure we've correctly formatted the FRAM payload.
         *
         * Fixed length sizes:
         *      4  Header
         *   6x 2  Variable size markers
         *      1  Locked state
         *      2  Stats server port
         *      9  PIR calibration
         *     14  Door calibration
         *      8  Range calibration
         *      4  Relay calibration
         * ---------------------------
         *     54  Total fixed (not incl. string sizes)
         */
        size_t size = 54 + pin.length() + device_name.length() + wifi_ssid.length() + wifi_password.length() +
                      stats_server_addr.length() + listen_devices.length();

        uint8_t payload[size];
        uint8_t* ptr = payload;

        memcpy(ptr, current_settings_header, 4);
        ptr += 4;

        auto write_block = [&ptr](String const& s) {
            size_t item_size = s.length();
            memcpy(ptr, &item_size, 2);
            memcpy(ptr + 2, s.c_str(), s.length());
            ptr += item_size + 2;
        };

        auto write_var = [&ptr](void* value, size_t sz) {
            memcpy(ptr, value, sz);
            ptr += sz;
        };

        write_var(&locked, 1);
        write_block(pin);
        write_block(device_name);
        write_block(wifi_ssid);
        write_block(wifi_password);

        write_block(stats_server_addr);
        write_var(&stats_server_port, 2);

        write_var(&pir_min_pixels, 1);
        write_var(&pir_pixel_delta, 4);
        write_var(&pir_total_delta, 4);

        write_var(&door_open_distance, 2);
        write_var(&door_open_wait, 4);
        write_var(&door_cool_down, 4);
        write_var(&door_close_ticks, 4);

        write_var(&range_trigger_threshold, 2);
        write_var(&range_fixed_calibration, 2);
        write_var(&range_trigger_coefficient, 4);

        write_var(&relay_activation_time, 4);

        write_block(listen_devices);

        if (size == (ptr - payload)) {
            ram.write(0, payload, size);
            logln("Settings written to FRAM", dosa::LogLevel::INFO);
        } else {
            logln("FRAM payload size mismatch", dosa::LogLevel::ERROR);
        }
    }

    void setDefaults()
    {
        // Common
        locked = LockState::UNLOCKED;
        pin = default_pin;
        device_name = "DOSA " + String(random(1000, 9999));
        wifi_ssid = null_str;
        wifi_password = null_str;
        listen_devices = null_str;
        stats_server_addr = null_str;
        stats_server_port = zero_16;

        // IR grid specific
        pir_min_pixels = default_pir_min_pixels;
        pir_pixel_delta = default_pir_pixel_delta;
        pir_total_delta = default_pir_total_delta;

        // Door winch specific
        door_open_distance = default_door_open_distance;
        door_open_wait = default_door_open_wait;
        door_cool_down = default_door_cool_down;
        door_close_ticks = default_door_close_ticks;

        // Range-trip specific
        range_trigger_threshold = default_range_trigger_threshold;
        range_fixed_calibration = default_range_fixed_calibration;
        range_trigger_coefficient = default_range_trigger_coefficient;

        // Relay specific
        relay_activation_time = default_relay_activation_time;

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
        if (newPin.length() < 4 || newPin.length() > 50) {
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

    [[nodiscard]] comms::Node getStatsServer() const
    {
        IPAddress addr;
        addr.fromString(getStatsServerAddr());
        return {addr, getStatsServerPort()};
    }

    void setStatsServer(comms::Node const& statsServer)
    {
        setStatsServerAddr(comms::ipToString(statsServer.ip));
        setStatsServerPort(statsServer.port);
    }

    [[nodiscard]] String const& getStatsServerAddr() const
    {
        return stats_server_addr;
    }

    [[nodiscard]] bool hasStatsServer() const
    {
        return stats_server_addr.length() > 0;
    }

    void setStatsServerAddr(String const& statsServerAddr)
    {
        stats_server_addr = statsServerAddr;
    }

    [[nodiscard]] uint16_t getStatsServerPort() const
    {
        return stats_server_port;
    }

    void setStatsServerPort(uint16_t statsServerPort)
    {
        stats_server_port = statsServerPort;
    }

    [[nodiscard]] uint8_t getPirMinPixels() const
    {
        return pir_min_pixels;
    }

    void setPirMinPixels(uint8_t value)
    {
        pir_min_pixels = value;
    }

    [[nodiscard]] float getPirPixelDelta() const
    {
        return pir_pixel_delta;
    }

    void setPirPixelDelta(float value)
    {
        pir_pixel_delta = value;
    }

    [[nodiscard]] float getPirTotalDelta() const
    {
        return pir_total_delta;
    }

    void setPirTotalDelta(float value)
    {
        pir_total_delta = value;
    }

    [[nodiscard]] uint32_t getDoorOpenWait() const
    {
        return door_open_wait;
    }

    void setDoorOpenWait(uint32_t doorOpenWait)
    {
        door_open_wait = doorOpenWait;
    }

    [[nodiscard]] uint32_t getDoorCoolDown() const
    {
        return door_cool_down;
    }

    void setDoorCoolDown(uint32_t doorCoolDown)
    {
        door_cool_down = doorCoolDown;
    }

    [[nodiscard]] uint16_t getDoorOpenDistance() const
    {
        return door_open_distance;
    }

    void setDoorOpenDistance(uint16_t doorOpenDistance)
    {
        door_open_distance = doorOpenDistance;
    }

    [[nodiscard]] uint32_t getDoorCloseTicks() const
    {
        return door_close_ticks;
    }

    void setDoorCloseTicks(uint32_t doorCloseTicks)
    {
        door_close_ticks = doorCloseTicks;
    }

    [[nodiscard]] uint16_t getRangeTriggerThreshold() const
    {
        return range_trigger_threshold;
    }

    void setRangeTriggerThreshold(uint16_t value)
    {
        range_trigger_threshold = value;
    }

    [[nodiscard]] uint16_t getRangeFixedCalibration() const
    {
        return range_fixed_calibration;
    }

    void setRangeFixedCalibration(uint16_t value)
    {
        range_fixed_calibration = value;
    }

    [[nodiscard]] float getRangeTriggerCoefficient() const
    {
        return range_trigger_coefficient;
    }

    void setRangeTriggerCoefficient(float value)
    {
        range_trigger_coefficient = value;
    }

    [[nodiscard]] LockState getLockState() const
    {
        return locked;
    }

    void setLockState(LockState state)
    {
        locked = state;
    }

    [[nodiscard]] uint32_t getRelayActivationTime() const
    {
        return relay_activation_time;
    }

    void setRelayActivationTime(uint32_t t)
    {
        relay_activation_time = t;
    }

    void addListenDevice(String const& v)
    {
        if (!hasListenDevice(v)) {
            listen_devices += v + '\n';
        }
    }

    void setListenDevices(String const& v)
    {
        listen_devices = v;
    }

    [[nodiscard]] String const& getListenDevices() const
    {
        return listen_devices;
    }

    [[nodiscard]] bool isListenForAllDevices() const
    {
        return listen_devices.length() == 0;
    }

    [[nodiscard]] bool hasListenDevice(String const& v) const
    {
        int index, pos = 0;
        while ((index = listen_devices.indexOf('\n', pos)) != -1) {
            if (listen_devices.substring(pos, index) == v) {
                return true;
            }
            pos = index + 1;
        }
        return false;
    }

    /**
     * If you're using the wifi, it may interrupt the SPI bus. You will need to re-init the FRAM chip before doing
     * anything if the wifi has been used.
     */
    void reInitRam()
    {
        ram.init();
    }

   protected:
    Fram& ram;
    String device_name;
    String listen_devices;
    char device_name_bytes[20] = {0};
    String pin;
    String wifi_ssid;
    String wifi_password;
    String stats_server_addr;
    uint16_t stats_server_port;
    uint8_t pir_min_pixels = 0;
    float pir_pixel_delta = 0;
    float pir_total_delta = 0;
    LockState locked = LockState::UNLOCKED;
    uint16_t door_open_distance = 0;
    uint32_t door_open_wait = 0;
    uint32_t door_cool_down = 0;
    uint32_t door_close_ticks = 0;
    uint16_t range_trigger_threshold = 0;
    uint16_t range_fixed_calibration = 0;
    float range_trigger_coefficient = 0;
    uint32_t relay_activation_time = 0;

    [[nodiscard]] static uint8_t getSettingsVersion(String const& version)
    {
        if (version.length() != 4 || version.substring(0, 2) != "DS") {
            return 0;
        }

        return version.substring(2).toInt();
    }

    /**
     * Check if we can upgrade the settings config from given version, instead of wiping clean the entire settings.
     */
    virtual bool canUpgrade(String const& version)
    {
        auto v = getSettingsVersion(version);
        return v >= 17 && v <= getSettingsVersion(current_settings_header);
    }

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
        ram.read(addr, &size, 2);

        if (size == 0) {
            return "";
        } else if (size > 500) {
            return DOSA_SETTINGS_OVERSIZE_READ;
        } else {
            char buffer[size + 1];
            ram.read(addr + 2, buffer, size);
            buffer[size] = 0;
            return {buffer};
        }
    }
};

}  // namespace dosa
