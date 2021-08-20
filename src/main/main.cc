/**
 * Main unit.
 *
 * This programme should be burnt to the main unit that is responsible for
 * driving the winch.
 */

#include <Arduino.h>
#include <ArduinoBLE.h>
#include <dosa.h>

char const* sensorServiceId = "19b10000-e8f2-537e-4f6c-d104768a1214";
char const* sensorCharacteristicId = "19b10001-e8f2-537e-4f6c-d104768a1214";
int const max_devices = 5;

class DevicePool
{
   public:
    DevicePool(DevicePool const&) = delete;
    void operator=(DevicePool const&) = delete;

    static DevicePool& getInstance()
    {
        static DevicePool instance;
        return instance;
    }

    BLEDevice& getDevice(unsigned short index)
    {
        return devices[index];
    }

    BLECharacteristic& getSensor(unsigned short index)
    {
        return sensors[index];
    }

    unsigned short& getState(unsigned short index)
    {
        return statuses[index];
    }

    bool exists(unsigned short index)
    {
        return devices[index];
    }

    void add(BLEDevice& device)
    {
        // Check if we already know about this device, if we do - the peripheral gave up on us :)
        for (unsigned short i = 0; i < max_devices; ++i) {
            BLEDevice& d = devices[i];
            if (d && (d.address() == device.address())) {
                disconnect(i);
            }
        }

        // Find a spare slot to add the device
        for (auto& d : devices) {
            if (!d) {
                if (connectToSensor(device)) {
                    d = device;
                }
                return;
            }
        }

        // Overflow
        auto& serial = dosa::SerialComms::getInstance();
        auto& lights = dosa::Lights::getInstance();

        serial.writeln(
            "Unable to register sensor (" + device.address() + "): hit device limit",
            dosa::LogLevel::WARNING);

        lights.off();
        errorSignal();
        delay(500);
    }

    void disconnect(unsigned short index)
    {
        auto& serial = dosa::SerialComms::getInstance();
        auto& device = devices[index];
        serial.writeln("Disconnect " + device.address());
        device.disconnect();
        device = BLEDevice();
        sensors[index] = BLECharacteristic();
        statuses[index] = 0;
    }

   private:
    static bool connectToSensor(BLEDevice& device)
    {
        auto& serial = dosa::SerialComms::getInstance();
        serial.write("Connecting to " + device.address() + "..");
        if (device.connect()) {
            serial.writeln(" success");
            return true;

        } else {
            serial.writeln(" failed");
            return false;
        }
    }

    BLEDevice devices[max_devices];
    BLECharacteristic sensors[max_devices];
    unsigned short statuses[max_devices] = {0};

    DevicePool() = default;
};

/**
 * Arduino setup
 */
void setup()
{
    auto& lights = dosa::Lights::getInstance();
    lights.setBuiltIn(true);

    auto& serial = dosa::SerialComms::getInstance();
    serial.setLogLevel(dosa::LogLevel::INFO);
    // serial.wait();

    serial.writeln("-- DOSA Driver Unit --");
    serial.writeln("Begin init..");

    // Bluetooth init
    auto& bt = dosa::Bluetooth::getInstance();
    if (!bt.setEnabled(true) || !bt.setName("DOSA-D " + bt.localAddress().substring(15))) {
        serial.writeln("Bluetooth init failed", dosa::LogLevel::CRITICAL);
        errorHoldingPattern();
    }

    bt.setConnectionInterval(500, 3200);  // 0.5 - 4 seconds
    bt.setAppearance(0x0741);             // powered gate
    bt.setAdvertise(true);

    // Init completed
    serial.writeln("Init complete\n");

    lights.off();
}

bool processDevice(unsigned const index)
{
    auto& serial = dosa::SerialComms::getInstance();
    auto& pool = DevicePool::getInstance();
    auto& device = pool.getDevice(index);
    auto& sensor = pool.getSensor(index);
    auto& state = pool.getState(index);

    if (device.connected()) {
        if (!sensor) {
            serial.write("Device " + device.address() + " discovery in progress..");
            if (device.discoverAttributes()) {
                sensor = device.characteristic(sensorCharacteristicId);
                if (!sensor) {
                    serial.writeln(" no sensor discovered, disconnecting");
                    pool.disconnect(index);
                    return false;
                } else {
                    serial.writeln(" ok");
                }
            } else {
                serial.writeln("");
            }

            // Only perform a single action else we risk timeout with other devices
            return true;
        }

        unsigned short value = 0;
        sensor.readValue(value);
        if (value != state) {
            state = value;
            if (value == 0) {
                serial.writeln("Sensor " + device.address() + " read fail");
            } else {
                serial.writeln("Sensor " + device.address() + " new state: " + value);
            }
        } else if (value == 0) {
            // Double-failure
            pool.disconnect(index);
        }

        return true;
    } else {
        pool.disconnect(index);
        return false;
    }
}

/**
 * Arduino main loop
 */
void loop()
{
    auto& lights = dosa::Lights::getInstance();
    auto& bt = dosa::Bluetooth::getInstance();
    auto& serial = dosa::SerialComms::getInstance();
    auto& pool = DevicePool::getInstance();

    if (!bt.isEnabled()) {
        serial.writeln("BT not enabled!", dosa::LogLevel::CRITICAL);
        errorHoldingPattern();
    }

    // Process all device connections
    int connected = 0;
    for (unsigned i = 0; i < max_devices; ++i) {
        if (pool.exists(i) && processDevice(i)) {
            ++connected;
        }
    }

    // Visual indicator of number of devices connected
    if (connected == 0) {
        lights.off();
    } else if (connected == 1) {
        lights.blue();
    } else if (connected == 2) {
        lights.green();
    } else {
        lights.red();
    }

    // Scan for new devices to add
    auto device = bt.scanForService(sensorServiceId);
    if (device) {
        pool.add(device);
    }
}
