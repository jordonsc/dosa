#include <dosa_messages.h>
#include <gtest/gtest.h>

using namespace dosa::messages;

// This is the size the tests are expecting an ack message to be, if it does not match DOSA_COMMS_ACK_SIZE then the
// tests are out of date.
#define ASSUMED_TRIGGER_SIZE 92

#define TEST_DEVICE_NAME "Trigger-App"

class TriggerTest : public ::testing::Test
{
   protected:
    char device_name[20] = {0};

    void SetUp() override
    {
        // NB: should still be null-padded because we init'd the array with nulls
        memcpy(device_name, TEST_DEVICE_NAME, 11);
    }

    void TearDown() override {}
};

/**
 * A trigger is a message saying that a sensor or event has fired.
 */
TEST_F(TriggerTest, BasicTest)
{
    ASSERT_EQ(DOSA_COMMS_TRIGGER_SIZE, ASSUMED_TRIGGER_SIZE);

    uint8_t map[64] = {0};
    auto trigger = Trigger(TriggerDevice::SENSOR_GRID, map, device_name);
    EXPECT_NE(trigger.getMessageId(), 0);
    ASSERT_EQ(trigger.getPayloadSize(), ASSUMED_TRIGGER_SIZE);
    EXPECT_EQ(strcmp(trigger.getDeviceName(), TEST_DEVICE_NAME), 0);  // only works if name < 20 chars
    EXPECT_EQ(trigger.getDeviceType(), TriggerDevice::SENSOR_GRID);

    char payload[ASSUMED_TRIGGER_SIZE] = {0};
    auto msgId = trigger.getMessageId();
    auto payloadSize = trigger.getPayloadSize();
    auto deviceType = trigger.getDeviceType();

    std::memcpy(payload, &msgId, 2);
    std::memcpy(payload + 2, "trg", 3);  // NOLINT(bugprone-not-null-terminated-result)
    std::memcpy(payload + 5, &payloadSize, 2);
    std::memcpy(payload + 7, trigger.getDeviceName(), 20);
    std::memcpy(payload + 27, &deviceType, 1);

    EXPECT_EQ(strncmp(payload, trigger.getPayload(), ASSUMED_TRIGGER_SIZE), 0);

    auto triggerPacket = Trigger::fromPacket(payload, ASSUMED_TRIGGER_SIZE);
    EXPECT_EQ(triggerPacket.getMessageId(), trigger.getMessageId());
    EXPECT_EQ(triggerPacket.getDeviceType(), trigger.getDeviceType());
    EXPECT_EQ(strcmp(trigger.getDeviceName(), device_name), 0);  // works because of null-padding
    char cmdCode[4] = {0};                               // important: requires null termination for C-str comparison
    memcpy(cmdCode, triggerPacket.getCommandCode(), 3);  // NB: only copying 3-bytes (per-spec, no null termination)
    EXPECT_EQ(strcmp(cmdCode, "trg"), 0);
    EXPECT_TRUE(triggerPacket == trigger);
}
