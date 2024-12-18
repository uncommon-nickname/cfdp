#include "cfdp_core/pdu_tlv.hpp"
#include "cfdp_core/pdu_enums.hpp"
#include "cfdp_core/pdu_exceptions.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <span>
#include <string>
#include <vector>

using ::cfdp::pdu::exception::DecodeFromBytesException;
using ::cfdp::pdu::exception::PduConstructionException;

using ::cfdp::pdu::tlv::EntityId;
using ::cfdp::pdu::tlv::FilestoreRequest;
using ::cfdp::pdu::tlv::FilestoreRequestActionCode;
using ::cfdp::pdu::tlv::MessageToUser;

class FilestoreRequestTest : public testing::Test
{
  public:
    std::unique_ptr<FilestoreRequest> buildOneFileTlv(FilestoreRequestActionCode code)
    {
        return std::make_unique<FilestoreRequest>(code, "first");
    }
    std::unique_ptr<FilestoreRequest> buildTwoFileTlv(FilestoreRequestActionCode code)
    {
        return std::make_unique<FilestoreRequest>(code, "first", "second");
    }

  protected:
    static constexpr std::array<uint8_t, 9> encoded_create_one_file_frame = {0,   7,   0,   5,  102,
                                                                             105, 114, 115, 116};
    static constexpr std::array<uint8_t, 16> encoded_create_two_file_frame = {
        0, 14, 32, 5, 102, 105, 114, 115, 116, 6, 115, 101, 99, 111, 110, 100};
};

class MessageToUserTest : public testing::Test
{
  protected:
    static constexpr std::array<uint8_t, 7> encoded_frame = {2, 5, 104, 101, 108, 108, 111};
};

class EntityIdTest : public testing::Test
{
  protected:
    static constexpr std::array<uint8_t, 8> encoded_frame = {6, 6, 0, 0, 0, 0, 4, 87};
};

class FilestoreRequestDecodingException : public FilestoreRequestTest,
                                          public testing::WithParamInterface<std::vector<uint8_t>>
{};

INSTANTIATE_TEST_SUITE_P(FilestoreRequestTest, FilestoreRequestDecodingException,
                         testing::Values(std::vector<uint8_t>{0},
                                         std::vector<uint8_t>{0, 15, 0, 1, 102},
                                         std::vector<uint8_t>{0, 6, 0, 6, 2, 3, 4, 5},
                                         std::vector<uint8_t>{0, 9, 32, 4, 2, 3, 4, 5, 4, 2, 3},
                                         std::vector<uint8_t>{6, 3, 0, 1, 102}));

TEST_F(FilestoreRequestTest, TestEncodingOneFile)
{
    auto tlv     = buildOneFileTlv(FilestoreRequestActionCode::CreateFile);
    auto encoded = tlv->encodeToBytes();

    ASSERT_EQ(tlv->actionCode, FilestoreRequestActionCode::CreateFile);
    ASSERT_EQ(tlv->firstFileName, "first");
    ASSERT_FALSE(tlv->secondFileName.has_value());

    EXPECT_THAT(encoded, testing::ElementsAreArray(encoded_create_one_file_frame));
}

TEST_F(FilestoreRequestTest, TestEncodingTwoFile)
{
    auto tlv     = buildTwoFileTlv(FilestoreRequestActionCode::RenameFile);
    auto encoded = tlv->encodeToBytes();

    ASSERT_EQ(tlv->actionCode, FilestoreRequestActionCode::RenameFile);
    ASSERT_EQ(tlv->firstFileName, "first");
    ASSERT_TRUE(tlv->secondFileName.has_value());
    ASSERT_EQ(tlv->secondFileName.value(), "second");

    EXPECT_THAT(encoded, testing::ElementsAreArray(encoded_create_two_file_frame));
}

TEST_F(FilestoreRequestTest, TestEncodingOneFileWrongCode)
{
    ASSERT_THROW(buildOneFileTlv(FilestoreRequestActionCode::RenameFile), PduConstructionException);
}

TEST_F(FilestoreRequestTest, TestEncodingTwoFileWrongCode)
{
    ASSERT_THROW(buildTwoFileTlv(FilestoreRequestActionCode::CreateFile), PduConstructionException);
}

TEST_F(FilestoreRequestTest, TestDecodingOneFile)
{
    auto encoded = std::span<uint8_t const>{encoded_create_one_file_frame.begin(),
                                            encoded_create_one_file_frame.end()};
    auto tlv     = FilestoreRequest(encoded);

    ASSERT_EQ(tlv.actionCode, FilestoreRequestActionCode::CreateFile);
    ASSERT_EQ(tlv.firstFileName, "first");
    ASSERT_FALSE(tlv.secondFileName.has_value());
}

TEST_F(FilestoreRequestTest, TestDecodingTwoFile)
{
    auto encoded = std::span<uint8_t const>{encoded_create_two_file_frame.begin(),
                                            encoded_create_two_file_frame.end()};
    auto tlv     = FilestoreRequest(encoded);

    ASSERT_EQ(tlv.actionCode, FilestoreRequestActionCode::RenameFile);
    ASSERT_EQ(tlv.firstFileName, "first");
    ASSERT_TRUE(tlv.secondFileName.has_value());
    ASSERT_EQ(tlv.secondFileName, "second");
}

TEST_P(FilestoreRequestDecodingException, TestDecodingException)
{
    auto frame   = GetParam();
    auto encoded = std::span<uint8_t const>{frame.begin(), frame.end()};

    ASSERT_THROW(FilestoreRequest{encoded}, DecodeFromBytesException);
}

TEST_F(MessageToUserTest, TestEncoding)
{
    auto tlv     = MessageToUser("hello");
    auto encoded = tlv.encodeToBytes();

    ASSERT_EQ(tlv.message, "hello");
    EXPECT_THAT(encoded, testing::ElementsAreArray(encoded_frame));
}

TEST_F(MessageToUserTest, TestDecoding)
{
    auto encoded = std::span<uint8_t const>{encoded_frame.begin(), encoded_frame.end()};
    auto tlv     = MessageToUser(encoded);

    ASSERT_EQ(tlv.message, "hello");
}

TEST_F(MessageToUserTest, TestDecodingEmptyMemory)
{
    ASSERT_THROW(MessageToUser(std::span<uint8_t, 0>{}), DecodeFromBytesException);
}

TEST_F(MessageToUserTest, TestDecodingTooSmallEntityLength)
{
    auto encoded = std::span<uint8_t const>{encoded_frame.begin(), encoded_frame.end() - 1};
    ASSERT_THROW(MessageToUser{encoded}, DecodeFromBytesException);
}

TEST_F(MessageToUserTest, TestDecodingWrongType)
{
    std::array<uint8_t, 8> frame = {0, 6, 0, 0, 0, 0, 4, 87};
    auto encoded                 = std::span<uint8_t const>{frame.begin(), frame.end()};
    ASSERT_THROW(MessageToUser{encoded}, DecodeFromBytesException);
}

TEST_F(EntityIdTest, TestEncoding)
{
    auto tlv     = EntityId(6, 1111);
    auto encoded = tlv.encodeToBytes();

    ASSERT_EQ(tlv.lengthOfEntityID, 6);
    ASSERT_EQ(tlv.faultEntityID, 1111);
    EXPECT_THAT(encoded, testing::ElementsAreArray(encoded_frame));
}

TEST_F(EntityIdTest, TestDecoding)
{
    auto encoded = std::span<uint8_t const>{encoded_frame.begin(), encoded_frame.end()};
    auto tlv     = EntityId(encoded);

    ASSERT_EQ(tlv.lengthOfEntityID, 6);
    ASSERT_EQ(tlv.faultEntityID, 1111);
}

TEST_F(EntityIdTest, TestDecodingEmptyMemory)
{
    ASSERT_THROW(EntityId(std::span<uint8_t, 0>{}), DecodeFromBytesException);
}

TEST_F(EntityIdTest, TestDecodingTooSmallEntityLength)
{
    auto encoded = std::span<uint8_t const>{encoded_frame.begin(), encoded_frame.end() - 1};
    ASSERT_THROW(EntityId{encoded}, DecodeFromBytesException);
}

TEST_F(EntityIdTest, TestDecodingWrongType)
{
    std::array<uint8_t, 8> frame = {0, 6, 0, 0, 0, 0, 4, 87};
    auto encoded                 = std::span<uint8_t const>{frame.begin(), frame.end()};
    ASSERT_THROW(EntityId{encoded}, DecodeFromBytesException);
}
