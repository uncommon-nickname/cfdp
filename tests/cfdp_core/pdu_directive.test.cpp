#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cfdp_core/pdu_directive.hpp>
#include <cfdp_core/pdu_enums.hpp>
#include <cfdp_core/pdu_exceptions.hpp>

#include <cstdint>
#include <span>

using ::cfdp::pdu::exception::DecodeFromBytesException;
using ::cfdp::pdu::exception::PduConstructionException;

using ::cfdp::pdu::directive::Ack;
using ::cfdp::pdu::directive::Condition;
using ::cfdp::pdu::directive::Directive;
using ::cfdp::pdu::directive::EndOfFile;
using ::cfdp::pdu::directive::KeepAlive;
using ::cfdp::pdu::directive::TransactionStatus;
using ::cfdp::pdu::header::LargeFileFlag;

class KeepAliveTest : public testing::Test
{
  protected:
    static constexpr std::array<uint8_t, 9> encoded_large_frame = {12,  255, 255, 255, 255,
                                                                   255, 255, 255, 255};
    static constexpr std::array<uint8_t, 5> encoded_small_frame = {12, 255, 255, 255, 255};
};

class AckTest : public testing::Test
{
  protected:
    static constexpr std::array<uint8_t, 3> encoded_eof_ack_frame      = {6, 64, 34};
    static constexpr std::array<uint8_t, 3> encoded_finished_ack_frame = {6, 81, 34};
};

class EndOfFileTest : public testing::Test
{
  protected:
    static constexpr std::array<uint8_t, 10> encoded_small_no_error_frame = {
        4, 0, 66, 58, 53, 199, 255, 255, 255, 255};
    static constexpr std::array<uint8_t, 14> encoded_small_with_error_frame = {
        4, 96, 66, 58, 53, 199, 255, 255, 255, 255, 6, 2, 48, 57};
    static constexpr std::array<uint8_t, 14> encoded_large_no_error_frame = {
        4, 0, 66, 58, 53, 199, 255, 255, 255, 255, 255, 255, 255, 255};
    static constexpr std::array<uint8_t, 18> encoded_large_with_error_frame = {
        4, 96, 66, 58, 53, 199, 255, 255, 255, 255, 255, 255, 255, 255, 6, 2, 48, 57};
    ;
};

TEST_F(KeepAliveTest, TestEncodingSmallFile)
{
    auto pdu     = KeepAlive(UINT32_MAX);
    auto encoded = pdu.encodeToBytes();

    ASSERT_EQ(pdu.getLargeFileFlag(), LargeFileFlag::SmallFile);
    ASSERT_EQ(pdu.getRawSize(), sizeof(uint8_t) + sizeof(uint32_t));
    EXPECT_THAT(encoded, testing::ElementsAreArray(encoded_small_frame));
}

TEST_F(KeepAliveTest, TestEncodingLargeFile)
{
    auto pdu     = KeepAlive(UINT64_MAX);
    auto encoded = pdu.encodeToBytes();

    ASSERT_EQ(pdu.getLargeFileFlag(), LargeFileFlag::LargeFile);
    ASSERT_EQ(pdu.getRawSize(), sizeof(uint8_t) + sizeof(uint64_t));
    EXPECT_THAT(encoded, testing::ElementsAreArray(encoded_large_frame));
}

TEST_F(KeepAliveTest, TestDecodingSmallFile)
{
    auto encoded = std::span<uint8_t const>{encoded_small_frame.begin(), encoded_small_frame.end()};

    auto pdu = KeepAlive(encoded);

    ASSERT_EQ(pdu.getLargeFileFlag(), LargeFileFlag::SmallFile);
    ASSERT_EQ(pdu.getProgress(), UINT32_MAX);
}

TEST_F(KeepAliveTest, TestDecodingLargeFile)
{
    auto encoded = std::span<uint8_t const>{encoded_large_frame.begin(), encoded_large_frame.end()};

    auto pdu = KeepAlive(encoded);

    ASSERT_EQ(pdu.getLargeFileFlag(), LargeFileFlag::LargeFile);
    ASSERT_EQ(pdu.getProgress(), UINT64_MAX);
}

TEST_F(KeepAliveTest, TestDecodingTooShortByteStream)
{
    auto encoded =
        std::span<uint8_t const>{encoded_large_frame.begin(), encoded_large_frame.end() - 1};

    ASSERT_THROW(KeepAlive{encoded}, cfdp::pdu::exception::DecodeFromBytesException);
}

TEST_F(KeepAliveTest, TestDecodingWrongDirectiveCode)
{
    std::array<uint8_t, 5> encoded_frame = {13, 255, 255, 255, 255};

    auto encoded = std::span<uint8_t const>{encoded_frame.begin(), encoded_frame.end() - 1};

    ASSERT_THROW(KeepAlive{encoded}, cfdp::pdu::exception::DecodeFromBytesException);
}

TEST_F(AckTest, TestConstructorException)
{
    ASSERT_THROW(
        Ack(Directive::Ack, Condition::KeepAliveLimitReached, TransactionStatus::Terminated),
        PduConstructionException);
}

TEST_F(AckTest, TestEncodingEofAck)
{
    auto pdu = Ack(Directive::Eof, Condition::KeepAliveLimitReached, TransactionStatus::Terminated);
    auto encoded = pdu.encodeToBytes();

    ASSERT_EQ(pdu.getDirectiveCode(), Directive::Eof);
    ASSERT_EQ(pdu.getConditionCode(), Condition::KeepAliveLimitReached);
    ASSERT_EQ(pdu.getTransactionStatus(), TransactionStatus::Terminated);

    EXPECT_THAT(encoded, testing::ElementsAreArray(encoded_eof_ack_frame));
}

TEST_F(AckTest, TestEncodingFinishedAck)
{
    auto pdu =
        Ack(Directive::Finished, Condition::KeepAliveLimitReached, TransactionStatus::Terminated);
    auto encoded = pdu.encodeToBytes();

    ASSERT_EQ(pdu.getDirectiveCode(), Directive::Finished);
    ASSERT_EQ(pdu.getConditionCode(), Condition::KeepAliveLimitReached);
    ASSERT_EQ(pdu.getTransactionStatus(), TransactionStatus::Terminated);

    EXPECT_THAT(encoded, testing::ElementsAreArray(encoded_finished_ack_frame));
}

TEST_F(AckTest, TestDecodingEofAck)
{
    auto encoded =
        std::span<uint8_t const>{encoded_eof_ack_frame.begin(), encoded_eof_ack_frame.end()};

    auto pdu = Ack(encoded);

    ASSERT_EQ(pdu.getDirectiveCode(), Directive::Eof);
    ASSERT_EQ(pdu.getConditionCode(), Condition::KeepAliveLimitReached);
    ASSERT_EQ(pdu.getTransactionStatus(), TransactionStatus::Terminated);
}

TEST_F(AckTest, TestDecodingFinishedAck)
{
    auto encoded = std::span<uint8_t const>{encoded_finished_ack_frame.begin(),
                                            encoded_finished_ack_frame.end()};

    auto pdu = Ack(encoded);

    ASSERT_EQ(pdu.getDirectiveCode(), Directive::Finished);
    ASSERT_EQ(pdu.getConditionCode(), Condition::KeepAliveLimitReached);
    ASSERT_EQ(pdu.getTransactionStatus(), TransactionStatus::Terminated);
}

TEST_F(AckTest, TestDecodingWrongByteStreamSize)
{
    auto encoded =
        std::span<uint8_t const>{encoded_eof_ack_frame.begin(), encoded_eof_ack_frame.end() - 1};

    ASSERT_THROW(Ack{encoded}, cfdp::pdu::exception::DecodeFromBytesException);
}

TEST_F(AckTest, TestDecodingWrongDirectiveCode)
{
    std::array<uint8_t, 3> encoded_frame = {12, 81, 34};

    auto encoded = std::span<uint8_t const>{encoded_frame.begin(), encoded_frame.end()};

    ASSERT_THROW(Ack{encoded}, cfdp::pdu::exception::DecodeFromBytesException);
}

TEST_F(EndOfFileTest, TestConstructorException)
{
    ASSERT_THROW(EndOfFile<uint32_t>(Condition::FileSizeError, 1, 1), PduConstructionException);

    ASSERT_THROW(EndOfFile<uint32_t>(Condition::NoError, 1, 1, 1, 1), PduConstructionException);
}

TEST_F(EndOfFileTest, TestEncodingSmallFileWithNoError)
{
    auto pdu     = EndOfFile(Condition::NoError, 1111111111, UINT32_MAX);
    auto encoded = pdu.encodeToBytes();

    ASSERT_EQ(pdu.getConditionCode(), Condition::NoError);
    ASSERT_EQ(pdu.getFileSize(), UINT32_MAX);
    ASSERT_EQ(pdu.getChecksum(), 1111111111);
    ASSERT_EQ(pdu.getLengthOfEntityID(), 0);
    ASSERT_EQ(pdu.getFaultEntityID(), 0);

    EXPECT_THAT(encoded, testing::ElementsAreArray(encoded_small_no_error_frame));
}

TEST_F(EndOfFileTest, TestEncodingLargeFileWithNoError)
{
    auto pdu     = EndOfFile(Condition::NoError, 1111111111, UINT64_MAX);
    auto encoded = pdu.encodeToBytes();

    ASSERT_EQ(pdu.getConditionCode(), Condition::NoError);
    ASSERT_EQ(pdu.getFileSize(), UINT64_MAX);
    ASSERT_EQ(pdu.getChecksum(), 1111111111);
    ASSERT_EQ(pdu.getLengthOfEntityID(), 0);
    ASSERT_EQ(pdu.getFaultEntityID(), 0);

    EXPECT_THAT(encoded, testing::ElementsAreArray(encoded_large_no_error_frame));
}

TEST_F(EndOfFileTest, TestEncodingSmallFileWithError)
{
    auto pdu     = EndOfFile(Condition::FileSizeError, 1111111111, UINT32_MAX, 2, 12345);
    auto encoded = pdu.encodeToBytes();

    ASSERT_EQ(pdu.getConditionCode(), Condition::FileSizeError);
    ASSERT_EQ(pdu.getLengthOfEntityID(), 2);
    ASSERT_EQ(pdu.getFaultEntityID(), 12345);

    EXPECT_THAT(encoded, testing::ElementsAreArray(encoded_small_with_error_frame));
}

TEST_F(EndOfFileTest, TestEncodingLargeFileWithError)
{
    auto pdu     = EndOfFile(Condition::FileSizeError, 1111111111, UINT64_MAX, 2, 12345);
    auto encoded = pdu.encodeToBytes();

    ASSERT_EQ(pdu.getConditionCode(), Condition::FileSizeError);
    ASSERT_EQ(pdu.getLengthOfEntityID(), 2);
    ASSERT_EQ(pdu.getFaultEntityID(), 12345);

    EXPECT_THAT(encoded, testing::ElementsAreArray(encoded_large_with_error_frame));
}

TEST_F(EndOfFileTest, TestDecodingSmallFileWithNoError)
{
    auto encoded = std::span<uint8_t const>{encoded_small_no_error_frame.begin(),
                                            encoded_small_no_error_frame.end()};

    auto pdu = EndOfFile<uint32_t>(encoded);

    ASSERT_EQ(pdu.getConditionCode(), Condition::NoError);
    ASSERT_EQ(pdu.getFileSize(), UINT32_MAX);
    ASSERT_EQ(pdu.getChecksum(), 1111111111);
    ASSERT_EQ(pdu.getLengthOfEntityID(), 0);
    ASSERT_EQ(pdu.getFaultEntityID(), 0);
}

TEST_F(EndOfFileTest, TestDecodingLargeFileWithNoError)
{
    auto encoded = std::span<uint8_t const>{encoded_large_no_error_frame.begin(),
                                            encoded_large_no_error_frame.end()};

    auto pdu = EndOfFile<uint64_t>(encoded);

    ASSERT_EQ(pdu.getConditionCode(), Condition::NoError);
    ASSERT_EQ(pdu.getFileSize(), UINT64_MAX);
    ASSERT_EQ(pdu.getChecksum(), 1111111111);
    ASSERT_EQ(pdu.getLengthOfEntityID(), 0);
    ASSERT_EQ(pdu.getFaultEntityID(), 0);
}

TEST_F(EndOfFileTest, TestDecodingSmallFileWithError)
{
    auto encoded = std::span<uint8_t const>{encoded_small_with_error_frame.begin(),
                                            encoded_small_with_error_frame.end()};

    auto pdu = EndOfFile<uint32_t>(encoded);

    ASSERT_EQ(pdu.getConditionCode(), Condition::FileSizeError);
    ASSERT_EQ(pdu.getLengthOfEntityID(), 2);
    ASSERT_EQ(pdu.getFaultEntityID(), 12345);
}

TEST_F(EndOfFileTest, TestDecodingLargeFileWithError)
{
    auto encoded = std::span<uint8_t const>{encoded_large_with_error_frame.begin(),
                                            encoded_large_with_error_frame.end()};

    auto pdu = EndOfFile<uint64_t>(encoded);

    ASSERT_EQ(pdu.getConditionCode(), Condition::FileSizeError);
    ASSERT_EQ(pdu.getLengthOfEntityID(), 2);
    ASSERT_EQ(pdu.getFaultEntityID(), 12345);
}

TEST_F(EndOfFileTest, TestDecodingWrongByteStreamSize)
{
    auto encoded = std::span<uint8_t const>{encoded_small_no_error_frame.begin(),
                                            encoded_small_no_error_frame.end() - 1};

    ASSERT_THROW(EndOfFile<uint32_t>{encoded}, DecodeFromBytesException);
}

TEST_F(EndOfFileTest, TestDecodingWrongTLVType)
{
    std::array<uint8_t, 14> encoded_frame = {4,   96,  66,  58, 53, 199, 255,
                                             255, 255, 255, 5,  2,  48,  57};
    auto encoded = std::span<uint8_t const>{encoded_frame.begin(), encoded_frame.end()};

    ASSERT_THROW(EndOfFile<uint32_t>{encoded}, DecodeFromBytesException);
}
