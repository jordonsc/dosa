#include <dosa_messages.h>
#include <gtest/gtest.h>

#include <string>

using namespace dosa::messages;

#define TEST_DEVICE_NAME "Stat-App"

class StatTest : public ::testing::Test
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

TEST_F(StatTest, BasicTest)
{
    auto msg = StatusMessage(12, "123456", 6, device_name);
    EXPECT_EQ(msg.getStatusFormat(), 12);
    ASSERT_EQ(msg.getStatusSize(), 6);
    EXPECT_EQ(std::string(msg.getStatusMessage(), msg.getStatusSize()), "123456");
}

TEST_F(StatTest, StatFromPayload)
{
    auto original = StatusMessage(5001, "abcd", 4, device_name);
    auto from_payload = StatusMessage::fromPacket(original.getPayload(), original.getPayloadSize());

    EXPECT_EQ(from_payload.getStatusFormat(), 5001);
    ASSERT_EQ(from_payload.getStatusSize(), 4);
    EXPECT_EQ(std::string(from_payload.getStatusMessage(), from_payload.getStatusSize()), "abcd");
}
