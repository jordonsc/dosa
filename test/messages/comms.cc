#include <gtest/gtest.h>
#include <messages.h>

using namespace dosa::messages;

// This is the size the tests are expecting an ack message to be, if it does not match DOSA_COMMS_ACK_SIZE then the
// tests are out of date.
#define ASSUMED_ACK_SIZE 9

/**
 * An ack is a generic acknowledgement of any given message. It contains only the original message ID (+2 bytes).
 */
TEST(MessagesTest, AckTest)
{
    ASSERT_EQ(DOSA_COMMS_ACK_SIZE, ASSUMED_ACK_SIZE);

    auto ack = Ack(1000);
    EXPECT_EQ(ack.getAckMsgId(), 1000);
    EXPECT_NE(ack.getMessageId(), 0);
    ASSERT_EQ(ack.getPayloadSize(), ASSUMED_ACK_SIZE);

    char payload[9] = {0};
    auto msgId = ack.getMessageId();
    auto payloadSize = ack.getPayloadSize();
    auto ackMsgId = ack.getAckMsgId();

    std::memcpy(payload, &msgId, 2);
    std::memcpy(payload + 2, "ack", 3);  // NOLINT(bugprone-not-null-terminated-result)
    std::memcpy(payload + 5, &payloadSize, 2);
    std::memcpy(payload + 7, &ackMsgId, 2);

    auto ackPacket = Ack::fromPacket(payload);
    EXPECT_EQ(ackPacket.getMessageId(), ack.getMessageId());
    EXPECT_EQ(ackPacket.getAckMsgId(), ack.getAckMsgId());
    EXPECT_TRUE(ackPacket == ack);

    auto genMsg = GenericMessage(payload, ack.getPayloadSize());
    EXPECT_EQ(genMsg.getMessageId(), ack.getMessageId());
    EXPECT_EQ(genMsg.getCommandCode(), "ack");
    EXPECT_EQ(genMsg.getPayloadSize(), ASSUMED_ACK_SIZE);
    EXPECT_EQ(genMsg.getMessageSize(), 2);

    uint16_t genAckMsgId;
    memcpy(&genAckMsgId, genMsg.getMessage(), 2);
    EXPECT_EQ(genAckMsgId, 1000);
}
