#include <dosa_messages.h>
#include <gtest/gtest.h>

using namespace dosa::messages;

#define TEST_DEVICE_NAME "Alt-App"

class AltTest : public ::testing::Test
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
TEST_F(AltTest, BasicTest)
{
    auto alt = Alt(10, device_name);
    EXPECT_NE(alt.getMessageId(), 0);
    ASSERT_EQ(alt.getPayloadSize(), DOSA_COMMS_ALT_SIZE);
    EXPECT_EQ(strcmp(alt.getDeviceName(), TEST_DEVICE_NAME), 0);  // only works if name < 20 chars
    EXPECT_EQ(alt.getCode(), 10);

    char payload[DOSA_COMMS_ALT_SIZE] = {0};
    auto msgId = alt.getMessageId();
    auto payloadSize = alt.getPayloadSize();
    auto code = alt.getCode();

    std::memcpy(payload, &msgId, 2);
    std::memcpy(payload + 2, "alt", 3);  // NOLINT(bugprone-not-null-terminated-result)
    std::memcpy(payload + 5, &payloadSize, 2);
    std::memcpy(payload + 7, alt.getDeviceName(), 20);
    std::memcpy(payload + DOSA_COMMS_PAYLOAD_BASE_SIZE, &code, sizeof(code));

    EXPECT_EQ(strncmp(payload, alt.getPayload(), DOSA_COMMS_ALT_SIZE), 0);

    auto altPacket = Alt::fromPacket(payload, DOSA_COMMS_ALT_SIZE);
    EXPECT_EQ(altPacket.getMessageId(), alt.getMessageId());
    EXPECT_EQ(altPacket.getCode(), alt.getCode());
    EXPECT_EQ(strcmp(alt.getDeviceName(), device_name), 0);  // works because of null-padding
    char cmdCode[4] = {0};                               // important: requires null termination for C-str comparison
    memcpy(cmdCode, altPacket.getCommandCode(), 3);  // NB: only copying 3-bytes (per-spec, no null termination)
    EXPECT_EQ(strcmp(cmdCode, "alt"), 0);
    EXPECT_TRUE(altPacket == alt);
}
