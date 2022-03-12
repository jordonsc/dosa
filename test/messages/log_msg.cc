#include <dosa_messages.h>
#include <gtest/gtest.h>

#include <string>

using namespace dosa::messages;

#define TEST_DEVICE_NAME "LogMsg-App"

class LogTest : public ::testing::Test
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

TEST_F(LogTest, BasicTest)
{
    {
        auto msg = LogMessage("log message", device_name);
        EXPECT_EQ(msg.getMessageSize(), 11);
        EXPECT_EQ(std::string(msg.getMessage(), msg.getMessageSize()), "log message");
        EXPECT_EQ(msg.getLogLevel(), LogMessageLevel::INFO);
    }

    {
        auto msg = LogMessage("omg error!", device_name, LogMessageLevel::ERROR);
        EXPECT_EQ(msg.getMessageSize(), 10);
        EXPECT_EQ(std::string(msg.getMessage(), msg.getMessageSize()), "omg error!");
        EXPECT_EQ(msg.getLogLevel(), LogMessageLevel::ERROR);
    }
}

TEST_F(LogTest, LogFromPayload)
{
    auto original = LogMessage("meep morp", device_name, LogMessageLevel::CRITICAL);
    auto from_payload = LogMessage::fromPacket(original.getPayload(), original.getPayloadSize());

    EXPECT_EQ(from_payload.getMessageSize(), 9);
    EXPECT_EQ(std::string(from_payload.getMessage(), from_payload.getMessageSize()), "meep morp");
    EXPECT_EQ(from_payload.getLogLevel(), LogMessageLevel::CRITICAL);
}
