#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cfdp_core/pdu_exceptions.hpp>
#include <cfdp_core/utils.hpp>

#include <array>
#include <span>
#include <vector>

using ::testing::ElementsAreArray;

using cfdp::pdu::exception::DecodeFromBytesException;
using cfdp::pdu::exception::EncodeToBytesException;

using ::cfdp::utils::bytesNeeded;
using ::cfdp::utils::bytesToInt;
using ::cfdp::utils::bytesToString;
using ::cfdp::utils::concatenateVectorsInplace;
using ::cfdp::utils::intToBytes;
using ::cfdp::utils::readLvValue;
using ::cfdp::utils::toUnderlying;

namespace
{
enum class TestEnum : uint8_t
{
    TestValue = 1,
};
}

TEST(CfdpUtils, TestIntToBytes)
{
    auto result = intToBytes(20, 4);

    EXPECT_THAT(result, ElementsAreArray(std::array<uint8_t, 4>{0, 0, 0, 20}));
}

TEST(CfdpUtils, TestIntToBytesSizeIsTooBig)
{
    EXPECT_THROW(intToBytes(20, 9), EncodeToBytesException);
}

TEST(CfdpUtils, TestBytesNeeded)
{
    auto result = bytesNeeded(UINT_MAX);

    ASSERT_EQ(result, 4);
}

TEST(CfdpUtils, TestBytesToInt)
{
    auto buff   = std::array<uint8_t const, 5>{0, 0, 0, 20, 1};
    auto memory = std::span<uint8_t const>{buff.begin(), buff.end()};

    auto result = bytesToInt<uint32_t>(memory, 0, 4);

    ASSERT_EQ(result, 20);
}

TEST(CfdpUtils, TestBytesToIntMemoryTooShort)
{
    auto buff   = std::array<uint8_t const, 5>{0, 0, 0, 20, 1};
    auto memory = std::span<uint8_t const>{buff.begin(), buff.end()};

    EXPECT_THROW(bytesToInt<uint32_t>(memory, 3, 4), DecodeFromBytesException);
}

TEST(CfdpUtils, TestBytesToIntTooSmallMemoryChunk)
{
    auto buff   = std::array<uint8_t const, 5>{0, 0, 0, 20, 1};
    auto memory = std::span<uint8_t const>{buff.begin(), buff.end()};

    EXPECT_THROW(bytesToInt<uint32_t>(memory, 0, 5), DecodeFromBytesException);
}

TEST(CfdpUtils, TestBytesToString)
{
    auto buff   = std::array<uint8_t const, 5>{104, 101, 108, 108, 111};
    auto memory = std::span<uint8_t const>{buff.begin(), buff.end()};

    auto result = bytesToString(memory, 0, 4);

    ASSERT_EQ(result, "hell");
}

TEST(CfdpUtils, TestBytesToStringTooShort)
{
    auto buff   = std::array<uint8_t const, 5>{104, 101, 108, 108, 111};
    auto memory = std::span<uint8_t const>{buff.begin(), buff.end()};

    EXPECT_THROW(bytesToString(memory, 3, 4), DecodeFromBytesException);
}

TEST(CfdpUtils, TestReadLvValue)
{
    auto buff   = std::array<uint8_t const, 5>{4, 0, 1, 2, 3};
    auto memory = std::span<uint8_t const>{buff.begin(), buff.end()};

    auto expected_value = std::span<uint8_t const>{buff.begin() + 1, buff.end()};

    auto result = readLvValue(memory, 0);

    ASSERT_EQ(result.size(), expected_value.size());

    for (auto i = 0; i < expected_value.size(); ++i)
    {
        ASSERT_EQ(result[i], expected_value[i]);
    }
}

TEST(CfdpUtils, TestReadLvValueMemoryTooShort)
{
    auto buff   = std::array<uint8_t const, 5>{5, 0, 1, 2, 3};
    auto memory = std::span<uint8_t const>{buff.begin(), buff.end()};

    EXPECT_THROW(readLvValue(memory, 0), DecodeFromBytesException);
}

TEST(CfdpUtils, TestConcatenateVectorsInplace)
{
    auto vec1 = std::vector<uint8_t>{1, 2, 3};
    auto vec2 = std::vector<uint8_t>{4, 5, 6};

    concatenateVectorsInplace(vec1, vec2);

    EXPECT_THAT(vec2, ElementsAreArray(std::array<uint8_t, 6>{4, 5, 6, 1, 2, 3}));
}

TEST(CfdpUtils, TestToUnderyling)
{
    auto value = TestEnum::TestValue;

    ASSERT_EQ(toUnderlying(value), 1);
}
