#include <gtest/gtest.h>
#include <messages.h>

using namespace dosa::messages;

// This is the size the tests are expecting an ack message to be, if it does not match DOSA_COMMS_ACK_SIZE then the
// tests are out of date.
#define ASSUMED_ACK_SIZE 29

#define TEST_DEVICE_NAME "Ack-App"

class AckTest : public ::testing::Test
{
   protected:
    char device_name[20] = {0};

    void SetUp() override
    {
        // NB: should still be null-padded because we init'd the array with nulls
        memcpy(device_name, TEST_DEVICE_NAME, 7);
    }

    void TearDown() override {}
};

/**
 * An ack is a generic acknowledgement of any given message. It contains only the original message ID (+2 bytes).
 */
TEST_F(AckTest, BasicTest)
{
    ASSERT_EQ(DOSA_COMMS_ACK_SIZE, ASSUMED_ACK_SIZE);

    auto ack = Ack(1000, device_name);
    EXPECT_EQ(ack.getAckMsgId(), 1000);
    EXPECT_NE(ack.getMessageId(), 0);
    ASSERT_EQ(ack.getPayloadSize(), ASSUMED_ACK_SIZE);
    EXPECT_EQ(strcmp(ack.getDeviceName(), TEST_DEVICE_NAME), 0);

    char payload[ASSUMED_ACK_SIZE] = {0};
    auto msgId = ack.getMessageId();
    auto payloadSize = ack.getPayloadSize();
    auto ackMsgId = ack.getAckMsgId();

    std::memcpy(payload, &msgId, 2);
    std::memcpy(payload + 2, "ack", 3);  // NOLINT(bugprone-not-null-terminated-result)
    std::memcpy(payload + 5, &payloadSize, 2);
    std::memcpy(payload + 7, ack.getDeviceName(), 20);
    std::memcpy(payload + 27, &ackMsgId, 2);

    EXPECT_EQ(strncmp(payload, ack.getPayload(), ASSUMED_ACK_SIZE), 0);

    auto ackPacket = Ack::fromPacket(payload);
    EXPECT_EQ(ackPacket.getMessageId(), ack.getMessageId());
    EXPECT_EQ(ackPacket.getAckMsgId(), ack.getAckMsgId());
    EXPECT_EQ(strcmp(ackPacket.getDeviceName(), device_name), 0);   // works because of null-padding
    char cmdCode[4] = {0};                           // important: requires null termination for C-str comparison
    memcpy(cmdCode, ackPacket.getCommandCode(), 3);  // NB: only copying 3-bytes (per-spec, no null termination)
    EXPECT_EQ(strcmp(cmdCode, "ack"), 0);
    EXPECT_TRUE(ackPacket == ack);

    auto genMsg = GenericMessage(payload, ack.getPayloadSize(), device_name);
    EXPECT_EQ(genMsg.getMessageId(), ack.getMessageId());
    EXPECT_EQ(std::string(genMsg.getCommandCode(), 3), "ack");
    EXPECT_EQ(genMsg.getPayloadSize(), ASSUMED_ACK_SIZE);
    EXPECT_EQ(genMsg.getMessageSize(), 2);

    uint16_t genAckMsgId;
    memcpy(&genAckMsgId, genMsg.getMessage(), 2);
    EXPECT_EQ(genAckMsgId, 1000);
}

TEST_F(AckTest, AckFromPayload)
{
    auto t = Trigger(TriggerDevice::IR_GRID, "FE");
    ASSERT_GT(t.getMessageId(), 0);

    auto ack = Ack(t, device_name);
    EXPECT_EQ(ack.getAckMsgId(), t.getMessageId());
    EXPECT_NE(ack.getMessageId(), t.getMessageId());
}
