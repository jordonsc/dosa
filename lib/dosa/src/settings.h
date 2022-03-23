#pragma once

#include <Arduino.h>

#define DOSA_SETTINGS_HEADER "DS21"
#define DOSA_SETTINGS_OVERSIZE_READ "#ERR-OVERSIZE"
#define DOSA_SETTINGS_DEFAULT_PIN "dosa"

/**
 * Minimum number of pixels that are considered 'changed' before we accept a trigger. Increase this to eliminate
 * single-pixel or edge anomalies.
 */
#define SENSOR_MIN_PIXELS_THRESHOLD 3

/**
 * Temp change (in Celsius) before considering any single pixel as "changed". This is a de-noising threshold, increase
 * this number to reduce the amount of noise the algorithm is sensitive to.
 */
#define SENSOR_SINGLE_DELTA_THRESHOLD 1.0

/**
 * The total temperature delta across all pixels before firing a trigger. This is the primary sensitivity metric, it
 * is also filtered against noise by SENSOR_SINGLE_DELTA_THRESHOLD so it won't show a true full-grid delta.
 */
#define SENSOR_TOTAL_DELTA_THRESHOLD 10.0

/**
 * Distance in mm the sonar should be <= when halting the door open sequence. The sonar should be reading the door's
 * distance from its apex/threshold.
 */
#define DOOR_OPEN_DISTANCE 500

/**
 * Time in milliseconds the door spends in then open-wait status, holding in an open position before closing again.
 */
#define DOOR_OPEN_WAIT_TIME 2000

/**
 * Time in milliseconds we wait before allowing further action after a trigger sequence.
 */
#define DOOR_COOL_DOWN 3000

/**
 * Fixed number of ticks we close the door for. This will translate to an approximate distance, it should be a small
 * amount greater than required.
 */
#define DOOR_CLOSE_TICKS 15000

/**
 * Number of consecutive reads with a reduced distance before firing the trigger.
 *
 * Increase to reduce noise.
 */
#define SONAR_TRIGGER_THRESHOLD 3

/**
 * Percentage of previous distance that's considered a trigger.
 */
#define SONAR_TRIGGER_COEFFICIENT 0.9

/**
 * Fixed distance for the sonar resting state. Set to zero for automatic detection.
 *
 * Recommended you set this value for outdoor devices, or devices aiming at non-perpendicular or non-solid surfaces.
 */
#define SONAR_FIXED_CALIBRATION 0

namespace dosa {

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
 *   1      uint8     Sensor cfg: SENSOR_MIN_PIXELS_THRESHOLD
 *   4      float     Sensor cfg: SENSOR_SINGLE_DELTA_THRESHOLD
 *   4      float     Sensor cfg: SENSOR_TOTAL_DELTA_THRESHOLD
 *   2      uint16    Door cfg: DOOR_OPEN_DISTANCE
 *   4      uint32    Door cfg: DOOR_OPEN_WAIT_TIME
 *   4      uint32    Door cfg: DOOR_COOL_DOWN
 *   4      uint32    Door cfg: DOOR_CLOSE_TICKS
 *   2      uint16    Sonar cfg: SONAR_TRIGGER_THRESHOLD
 *   2      uint16    Sonar cfg: SONAR_FIXED_CALIBRATION
 *   4      float     Sonar cfg: SONAR_TRIGGER_COEFFICIENT
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
        auto currentSettingsNumber = getSettingsVersion(currentSettingsVersion);
        bool upgrading = false;

        if (currentSettingsVersion != DOSA_SETTINGS_HEADER) {
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

        auto read_block = [this, &ptr, currentSettingsNumber](String& s, uint8_t req_ver = 0) -> bool {
            if (req_ver > 0 && currentSettingsNumber < req_ver) {
                return false;
            }

            s = readVarChar(ptr);
            if (s == DOSA_SETTINGS_OVERSIZE_READ) {
                logln("Oversize read warning", LogLevel::ERROR);
                return false;
            }
            ptr += s.length() + 2;
            return true;
        };

        auto read_var = [this, &ptr, currentSettingsNumber](void* s, size_t size, uint8_t req_ver = 0) -> bool {
            if (req_ver > 0 && currentSettingsNumber < req_ver) {
                return false;
            }

            ram.read(ptr, (uint8_t*)s, size);
            ptr += size;

            return true;
        };

        if (!read_var(&locked, 1, 20)) {
            locked = 0;
        }

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

        read_var(&sensor_min_pixels, 1);
        read_var(&sensor_pixel_delta, 4);
        read_var(&sensor_total_delta, 4);
        read_var(&door_open_distance, 2);
        read_var(&door_open_wait, 4);
        read_var(&door_cool_down, 4);
        read_var(&door_close_ticks, 4);
        read_var(&sonar_trigger_threshold, 2);

        if (!read_var(&sonar_fixed_calibration, 2, 18)) {
            sonar_fixed_calibration = SONAR_FIXED_CALIBRATION;
        }

        if (!read_var(&sonar_trigger_coefficient, 4, 19)) {
            sonar_trigger_coefficient = SONAR_TRIGGER_COEFFICIENT;
        }

        if (!read_block(listen_devices, 21)) {
            listen_devices = "";
        }

        // Validate values
        bool valid = true;

        if (pin == "") {
            // Not allowed a blank pin
            pin = DOSA_SETTINGS_DEFAULT_PIN;
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
         * Fixed length sizes:
         *      4  Header
         *      1  Locked state
         *   5x 2  Variable size markers
         *      9  Sensor calibration
         *     14  Door calibration
         *      8  Sonar calibration
         * ---------------------------
         *     46  Total
         */
        size_t size = 46 + pin.length() + device_name.length() + wifi_ssid.length() + wifi_password.length() +
                      listen_devices.length();

        uint8_t payload[size];
        uint8_t* ptr = payload;

        memcpy(ptr, DOSA_SETTINGS_HEADER, 4);
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

        write_var(&sensor_min_pixels, 1);
        write_var(&sensor_pixel_delta, 4);
        write_var(&sensor_total_delta, 4);

        write_var(&door_open_distance, 2);
        write_var(&door_open_wait, 4);
        write_var(&door_cool_down, 4);
        write_var(&door_close_ticks, 4);

        write_var(&sonar_trigger_threshold, 2);
        write_var(&sonar_fixed_calibration, 2);
        write_var(&sonar_trigger_coefficient, 4);

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
        locked = 0;
        pin = DOSA_SETTINGS_DEFAULT_PIN;
        device_name = "DOSA " + String(random(1000, 9999));
        wifi_ssid = "";
        wifi_password = "";
        listen_devices = "";

        // IR grid specific
        sensor_min_pixels = SENSOR_MIN_PIXELS_THRESHOLD;
        sensor_pixel_delta = SENSOR_SINGLE_DELTA_THRESHOLD;
        sensor_total_delta = SENSOR_TOTAL_DELTA_THRESHOLD;

        // Door winch specific
        door_open_distance = DOOR_OPEN_DISTANCE;
        door_open_wait = DOOR_OPEN_WAIT_TIME;
        door_cool_down = DOOR_COOL_DOWN;
        door_close_ticks = DOOR_CLOSE_TICKS;

        // Sonar specific
        sonar_trigger_threshold = SONAR_TRIGGER_THRESHOLD;
        sonar_fixed_calibration = SONAR_FIXED_CALIBRATION;
        sonar_trigger_coefficient = SONAR_TRIGGER_COEFFICIENT;

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

    [[nodiscard]] uint8_t getSensorMinPixels() const
    {
        return sensor_min_pixels;
    }

    void setSensorMinPixels(uint8_t sensorMinPixels)
    {
        sensor_min_pixels = sensorMinPixels;
    }

    [[nodiscard]] float getSensorPixelDelta() const
    {
        return sensor_pixel_delta;
    }

    void setSensorPixelDelta(float sensorPixelDelta)
    {
        sensor_pixel_delta = sensorPixelDelta;
    }

    [[nodiscard]] float getSensorTotalDelta() const
    {
        return sensor_total_delta;
    }

    void setSensorTotalDelta(float sensorTotalDelta)
    {
        sensor_total_delta = sensorTotalDelta;
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

    [[nodiscard]] uint16_t getSonarTriggerThreshold() const
    {
        return sonar_trigger_threshold;
    }

    void setSonarTriggerThreshold(uint16_t sonarTriggerThreshold)
    {
        sonar_trigger_threshold = sonarTriggerThreshold;
    }

    [[nodiscard]] uint16_t getSonarFixedCalibration() const
    {
        return sonar_fixed_calibration;
    }

    void setSonarFixedCalibration(uint16_t sonarFixedCalibration)
    {
        sonar_fixed_calibration = sonarFixedCalibration;
    }

    [[nodiscard]] float getSonarTriggerCoefficient() const
    {
        return sonar_trigger_coefficient;
    }

    void setSonarTriggerCoefficient(float sonarTriggerCoefficient)
    {
        sonar_trigger_coefficient = sonarTriggerCoefficient;
    }

    bool getLockState() const
    {
        return locked != 0;
    }

    void setLocked(bool lock_enabled)
    {
        locked = lock_enabled ? 1 : 0;
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
    uint8_t sensor_min_pixels = 0;
    float sensor_pixel_delta = 0;
    float sensor_total_delta = 0;
    uint8_t locked = 0;
    uint16_t door_open_distance = 0;
    uint32_t door_open_wait = 0;
    uint32_t door_cool_down = 0;
    uint32_t door_close_ticks = 0;
    uint16_t sonar_trigger_threshold = 0;
    uint16_t sonar_fixed_calibration = 0;
    float sonar_trigger_coefficient = 0.9;

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
        return v >= 17 && v <= getSettingsVersion(DOSA_SETTINGS_HEADER);
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
