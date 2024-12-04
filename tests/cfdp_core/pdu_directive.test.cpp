#include "cfdp_core/pdu_tlv.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cfdp_core/pdu_directive.hpp>
#include <cfdp_core/pdu_enums.hpp>
#include <cfdp_core/pdu_exceptions.hpp>

#include <cstdint>
#include <functional>
#include <span>
#include <tuple>

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
  public:
    std::unique_ptr<EndOfFile> buildNoErrorPdu(Condition conditionCode, uint64_t fileSize,
                                               LargeFileFlag largeFileFlag)
    {
        return std::make_unique<EndOfFile>(conditionCode, 1111111111, fileSize, largeFileFlag);
    }

    std::unique_ptr<EndOfFile> buildErrorPdu(Condition conditionCode, uint64_t fileSize,
                                             LargeFileFlag largeFileFlag, uint8_t lengthOfEntityID,
                                             uint64_t faultEntityID)
    {
        return std::make_unique<EndOfFile>(
            conditionCode, 1111111111, fileSize, largeFileFlag,
            std::make_unique<cfdp::pdu::tlv::EntityId>(lengthOfEntityID, faultEntityID));
    }

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

class EndOfFileNoErrorConstructorException
    : public EndOfFileTest,
      public testing::WithParamInterface<std::tuple<Condition, uint64_t, LargeFileFlag>>
{};

class EndOfFileErrorConstructorException
    : public EndOfFileTest,
      public testing::WithParamInterface<
          std::tuple<Condition, uint64_t, LargeFileFlag, uint8_t, uint64_t>>
{};

INSTANTIATE_TEST_SUITE_P(
    EndOfFileTest, EndOfFileNoErrorConstructorException,
    testing::Values(std::make_tuple(Condition::FileSizeError, 1, LargeFileFlag::SmallFile),
                    std::make_tuple(Condition::NoError, UINT64_MAX, LargeFileFlag::SmallFile)));

INSTANTIATE_TEST_SUITE_P(EndOfFileTest, EndOfFileErrorConstructorException,
                         testing::Values(std::make_tuple(Condition::NoError, 1,
                                                         LargeFileFlag::SmallFile, 1, 1),
                                         std::make_tuple(Condition::FileSizeError, UINT64_MAX,
                                                         LargeFileFlag::SmallFile, 1, 1)));

TEST_F(KeepAliveTest, TestEncodingSmallFile)
{
    auto pdu     = KeepAlive(UINT32_MAX, LargeFileFlag::SmallFile);
    auto encoded = pdu.encodeToBytes();

    ASSERT_EQ(pdu.largeFileFlag, LargeFileFlag::SmallFile);
    ASSERT_EQ(pdu.getRawSize(), sizeof(uint8_t) + sizeof(uint32_t));
    EXPECT_THAT(encoded, testing::ElementsAreArray(encoded_small_frame));
}

TEST_F(KeepAliveTest, TestEncodingLargeFile)
{
    auto pdu     = KeepAlive(UINT64_MAX, LargeFileFlag::LargeFile);
    auto encoded = pdu.encodeToBytes();

    ASSERT_EQ(pdu.largeFileFlag, LargeFileFlag::LargeFile);
    ASSERT_EQ(pdu.getRawSize(), sizeof(uint8_t) + sizeof(uint64_t));
    EXPECT_THAT(encoded, testing::ElementsAreArray(encoded_large_frame));
}

TEST_F(KeepAliveTest, TestEncodingWrongFileSize)
{
    ASSERT_THROW(KeepAlive(UINT64_MAX, LargeFileFlag::SmallFile), PduConstructionException);
}
TEST_F(KeepAliveTest, TestDecodingSmallFile)
{
    auto encoded = std::span<uint8_t const>{encoded_small_frame.begin(), encoded_small_frame.end()};

    auto pdu = KeepAlive(encoded);

    ASSERT_EQ(pdu.largeFileFlag, LargeFileFlag::SmallFile);
    ASSERT_EQ(pdu.progress, UINT32_MAX);
}

TEST_F(KeepAliveTest, TestDecodingLargeFile)
{
    auto encoded = std::span<uint8_t const>{encoded_large_frame.begin(), encoded_large_frame.end()};

    auto pdu = KeepAlive(encoded);

    ASSERT_EQ(pdu.largeFileFlag, LargeFileFlag::LargeFile);
    ASSERT_EQ(pdu.progress, UINT64_MAX);
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

    ASSERT_EQ(pdu.directiveCode, Directive::Eof);
    ASSERT_EQ(pdu.conditionCode, Condition::KeepAliveLimitReached);
    ASSERT_EQ(pdu.transactionStatus, TransactionStatus::Terminated);

    EXPECT_THAT(encoded, testing::ElementsAreArray(encoded_eof_ack_frame));
}

TEST_F(AckTest, TestEncodingFinishedAck)
{
    auto pdu =
        Ack(Directive::Finished, Condition::KeepAliveLimitReached, TransactionStatus::Terminated);
    auto encoded = pdu.encodeToBytes();

    ASSERT_EQ(pdu.directiveCode, Directive::Finished);
    ASSERT_EQ(pdu.conditionCode, Condition::KeepAliveLimitReached);
    ASSERT_EQ(pdu.transactionStatus, TransactionStatus::Terminated);

    EXPECT_THAT(encoded, testing::ElementsAreArray(encoded_finished_ack_frame));
}

TEST_F(AckTest, TestDecodingEofAck)
{
    auto encoded =
        std::span<uint8_t const>{encoded_eof_ack_frame.begin(), encoded_eof_ack_frame.end()};

    auto pdu = Ack(encoded);

    ASSERT_EQ(pdu.directiveCode, Directive::Eof);
    ASSERT_EQ(pdu.conditionCode, Condition::KeepAliveLimitReached);
    ASSERT_EQ(pdu.transactionStatus, TransactionStatus::Terminated);
}

TEST_F(AckTest, TestDecodingFinishedAck)
{
    auto encoded = std::span<uint8_t const>{encoded_finished_ack_frame.begin(),
                                            encoded_finished_ack_frame.end()};

    auto pdu = Ack(encoded);

    ASSERT_EQ(pdu.directiveCode, Directive::Finished);
    ASSERT_EQ(pdu.conditionCode, Condition::KeepAliveLimitReached);
    ASSERT_EQ(pdu.transactionStatus, TransactionStatus::Terminated);
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

TEST_P(EndOfFileNoErrorConstructorException, TestConstructorException)
{
    auto params = GetParam();
    ASSERT_THROW(
        std::apply(std::bind_front(&EndOfFileNoErrorConstructorException::buildNoErrorPdu, this),
                   params),
        PduConstructionException);
}

TEST_P(EndOfFileErrorConstructorException, TestConstructorException)
{
    auto params = GetParam();
    ASSERT_THROW(
        std::apply(std::bind_front(&EndOfFileNoErrorConstructorException::buildErrorPdu, this),
                   params),
        PduConstructionException);
}

TEST_F(EndOfFileTest, TestEncodingSmallFileWithoutError)
{
    auto pdu     = buildNoErrorPdu(Condition::NoError, UINT32_MAX, LargeFileFlag::SmallFile);
    auto encoded = pdu->encodeToBytes();

    ASSERT_EQ(pdu->conditionCode, Condition::NoError);
    ASSERT_EQ(pdu->fileSize, UINT32_MAX);
    ASSERT_EQ(pdu->checksum, 1111111111);
    ASSERT_FALSE(pdu->entityId.has_value());

    EXPECT_THAT(encoded, testing::ElementsAreArray(encoded_small_no_error_frame));
}

TEST_F(EndOfFileTest, TestEncodingLargeFileWithoutError)
{
    auto pdu     = buildNoErrorPdu(Condition::NoError, UINT64_MAX, LargeFileFlag::LargeFile);
    auto encoded = pdu->encodeToBytes();

    ASSERT_EQ(pdu->conditionCode, Condition::NoError);
    ASSERT_EQ(pdu->fileSize, UINT64_MAX);
    ASSERT_EQ(pdu->checksum, 1111111111);
    ASSERT_FALSE(pdu->entityId.has_value());

    EXPECT_THAT(encoded, testing::ElementsAreArray(encoded_large_no_error_frame));
}

TEST_F(EndOfFileTest, TestEncodingSmallFileWithError)
{
    auto pdu =
        buildErrorPdu(Condition::FileSizeError, UINT32_MAX, LargeFileFlag::SmallFile, 2, 12345);
    auto encoded = pdu->encodeToBytes();

    ASSERT_EQ(pdu->conditionCode, Condition::FileSizeError);
    ASSERT_TRUE(pdu->entityId.has_value());
    ASSERT_EQ(pdu->entityId->get()->lengthOfEntityID, 2);
    ASSERT_EQ(pdu->entityId->get()->faultEntityID, 12345);

    EXPECT_THAT(encoded, testing::ElementsAreArray(encoded_small_with_error_frame));
}

TEST_F(EndOfFileTest, TestEncodingLargeFileWithError)
{
    auto pdu =
        buildErrorPdu(Condition::FileSizeError, UINT64_MAX, LargeFileFlag::LargeFile, 2, 12345);
    auto encoded = pdu->encodeToBytes();

    ASSERT_EQ(pdu->conditionCode, Condition::FileSizeError);
    ASSERT_TRUE(pdu->entityId.has_value());
    ASSERT_EQ(pdu->entityId->get()->lengthOfEntityID, 2);
    ASSERT_EQ(pdu->entityId->get()->faultEntityID, 12345);

    EXPECT_THAT(encoded, testing::ElementsAreArray(encoded_large_with_error_frame));
}

TEST_F(EndOfFileTest, TestDecodingSmallFileWithNoError)
{
    auto encoded = std::span<uint8_t const>{encoded_small_no_error_frame.begin(),
                                            encoded_small_no_error_frame.end()};

    auto pdu = EndOfFile(encoded, LargeFileFlag::SmallFile);

    ASSERT_EQ(pdu.conditionCode, Condition::NoError);
    ASSERT_EQ(pdu.fileSize, UINT32_MAX);
    ASSERT_EQ(pdu.checksum, 1111111111);
    ASSERT_FALSE(pdu.entityId.has_value());
}

TEST_F(EndOfFileTest, TestDecodingLargeFileWithNoError)
{
    auto encoded = std::span<uint8_t const>{encoded_large_no_error_frame.begin(),
                                            encoded_large_no_error_frame.end()};

    auto pdu = EndOfFile(encoded, LargeFileFlag::LargeFile);

    ASSERT_EQ(pdu.conditionCode, Condition::NoError);
    ASSERT_EQ(pdu.fileSize, UINT64_MAX);
    ASSERT_EQ(pdu.checksum, 1111111111);
    ASSERT_FALSE(pdu.entityId.has_value());
}

TEST_F(EndOfFileTest, TestDecodingSmallFileWithError)
{
    auto encoded = std::span<uint8_t const>{encoded_small_with_error_frame.begin(),
                                            encoded_small_with_error_frame.end()};

    auto pdu = EndOfFile(encoded, LargeFileFlag::SmallFile);

    ASSERT_EQ(pdu.conditionCode, Condition::FileSizeError);
    ASSERT_TRUE(pdu.entityId.has_value());
    ASSERT_EQ(pdu.entityId->get()->lengthOfEntityID, 2);
    ASSERT_EQ(pdu.entityId->get()->faultEntityID, 12345);
}

TEST_F(EndOfFileTest, TestDecodingLargeFileWithError)
{
    auto encoded = std::span<uint8_t const>{encoded_large_with_error_frame.begin(),
                                            encoded_large_with_error_frame.end()};

    auto pdu = EndOfFile(encoded, LargeFileFlag::LargeFile);

    ASSERT_EQ(pdu.conditionCode, Condition::FileSizeError);
    ASSERT_TRUE(pdu.entityId.has_value());
    ASSERT_EQ(pdu.entityId->get()->lengthOfEntityID, 2);
    ASSERT_EQ(pdu.entityId->get()->faultEntityID, 12345);
}

TEST_F(EndOfFileTest, TestDecodingWrongByteStreamSize)
{
    auto encoded = std::span<uint8_t const>{encoded_small_no_error_frame.begin(),
                                            encoded_small_no_error_frame.end() - 1};

    ASSERT_THROW(EndOfFile(encoded, LargeFileFlag::SmallFile), DecodeFromBytesException);
}

TEST_F(EndOfFileTest, TestDecodingWrongTLVType)
{
    std::array<uint8_t, 14> encoded_frame = {4,   96,  66,  58, 53, 199, 255,
                                             255, 255, 255, 5,  2,  48,  57};
    auto encoded = std::span<uint8_t const>{encoded_frame.begin(), encoded_frame.end()};

    ASSERT_THROW(EndOfFile(encoded, LargeFileFlag::SmallFile), DecodeFromBytesException);
}
