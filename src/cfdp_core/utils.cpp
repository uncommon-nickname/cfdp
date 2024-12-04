#include <cfdp_core/utils.hpp>

#include <bit>
#include <cstdint>

std::vector<uint8_t> cfdp::utils::intToBytes(uint64_t value, uint8_t size)
{
    if (size > sizeof(uint64_t))
    {
        throw exception::EncodeToBytesException{"Size can't be larger than 8 bytes"};
    }

    std::span<uint8_t> view{std::bit_cast<uint8_t*>(&value), size};

    return std::vector<uint8_t>{view.rbegin(), view.rend()};
};

size_t cfdp::utils::bytesNeeded(uint64_t number)
{
    size_t bitsNeeded  = std::bit_width(number);
    size_t bytesNeeded = bitsNeeded == 0 ? 1 : (bitsNeeded + 7) / 8;

    return bytesNeeded;
}

std::string cfdp::utils::bytesToString(std::span<uint8_t const> memory, uint32_t offset,
                                       uint32_t size)
{
    if (memory.size() < offset + size)
    {
        throw exception::DecodeFromBytesException{"Passed memory does not contain enough bytes"};
    }

    auto subspan = memory.subspan(offset, size);

    std::string result(subspan.begin(), subspan.end());

    return result;
}

std::span<uint8_t const> cfdp::utils::readLvValue(std::span<uint8_t const> memory, uint32_t offset)
{
    if (memory.size() < offset + sizeof(uint8_t))
    {
        throw exception::DecodeFromBytesException{"Passed memory does not contain enough bytes"};
    }
    auto value_size = memory[offset];

    if (memory.size() < offset + sizeof(uint8_t) + value_size)
    {
        throw exception::DecodeFromBytesException{"Passed memory does not contain enough bytes"};
    }
    return memory.subspan(offset + 1, value_size);
}
