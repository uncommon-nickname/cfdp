#include "cfdp_core/pdu_tlv.hpp"
#include "cfdp_core/pdu_enums.hpp"
#include <cfdp_core/pdu_exceptions.hpp>
#include <cfdp_core/utils.hpp>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr uint8_t filestore_request_action_code_bitmask = 0b1111'0000;
} // namespace

namespace utils     = ::cfdp::utils;
namespace exception = ::cfdp::pdu::exception;

cfdp::pdu::tlv::FilestoreRequest::FilestoreRequest(FilestoreRequestActionCode actionCode,
                                                   std::string firstFileName)
    : actionCode(actionCode), firstFileName(std::move(firstFileName))
{
    if (shouldHaveSecondFile())
    {
        throw exception::PduConstructionException("This action should have second file");
    }
};
cfdp::pdu::tlv::FilestoreRequest::FilestoreRequest(FilestoreRequestActionCode actionCode,
                                                   std::string firstFileName,
                                                   std::string secondFileName)
    : actionCode(actionCode), firstFileName(std::move(firstFileName)),
      secondFileName(std::move(secondFileName))
{
    if (not shouldHaveSecondFile())
    {
        throw exception::PduConstructionException("This action shouldn't have second file");
    }
};

cfdp::pdu::tlv::FilestoreRequest::FilestoreRequest(std::span<uint8_t const> memory)
{
    const auto memory_size = memory.size();

    if (memory_size < sizeof(uint8_t) + sizeof(uint8_t))
    {
        throw exception::DecodeFromBytesException("Passed memory does not contain enough bytes");
    }

    if (memory[0] != utils::toUnderlying(TLVType::FilestoreRequest))
    {
        throw exception::DecodeFromBytesException("TLVType is not Filestore Request");
    }

    if (memory_size < sizeof(uint8_t) + sizeof(uint8_t) + memory[1])
    {
        throw exception::DecodeFromBytesException("Passed memory does not contain enough bytes");
    }

    actionCode =
        FilestoreRequestActionCode((memory[2] & filestore_request_action_code_bitmask) >> 4);

    auto firstFileNameBytes = utils::readLvValue(memory, 3);
    firstFileName = utils::bytesToString(firstFileNameBytes, 0, firstFileNameBytes.size());

    if (not shouldHaveSecondFile())
    {
        return;
    }

    auto secondFilePosition  = 4 + firstFileName.size();
    auto secondFileNameBytes = utils::readLvValue(memory, secondFilePosition);
    secondFileName = utils::bytesToString(secondFileNameBytes, 0, secondFileNameBytes.size());
};

std::vector<uint8_t> cfdp::pdu::tlv::FilestoreRequest::encodeToBytes() const
{
    const auto pdu_size = getRawSize();
    auto encodedTlv     = std::vector<uint8_t>{};

    encodedTlv.reserve(pdu_size);

    encodedTlv.push_back(utils::toUnderlying(TLVType::FilestoreRequest));
    encodedTlv.push_back(valueSize());
    encodedTlv.push_back((utils::toUnderlying(actionCode) << 4));
    encodedTlv.push_back(firstFileName.length());

    std::vector<uint8_t> firstFileNameBytes(firstFileName.begin(), firstFileName.end());

    utils::concatenateVectorsInplace(firstFileNameBytes, encodedTlv);

    if (not shouldHaveSecondFile())
    {
        return encodedTlv;
    }

    encodedTlv.push_back(secondFileName.value().length());
    std::vector<uint8_t> secondFileNameBytes(secondFileName->begin(), secondFileName->end());

    utils::concatenateVectorsInplace(secondFileNameBytes, encodedTlv);

    return encodedTlv;
}

cfdp::pdu::tlv::MessageToUser::MessageToUser(std::span<uint8_t const> memory)
{
    const auto memory_size = memory.size();

    if (memory_size < sizeof(uint8_t) + sizeof(uint8_t))
    {
        throw exception::DecodeFromBytesException("Passed memory does not contain enough bytes");
    }

    if (memory[0] != utils::toUnderlying(TLVType::MessageToUser))
    {
        throw exception::DecodeFromBytesException("TLVType is not Message To User");
    }

    const auto value_length = memory[1];

    if (memory_size < sizeof(uint8_t) + sizeof(uint8_t) + value_length)
    {
        throw exception::DecodeFromBytesException("Passed memory does not contain enough bytes");
    }

    message = utils::bytesToString(memory, 2, value_length);
};

std::vector<uint8_t> cfdp::pdu::tlv::MessageToUser::encodeToBytes() const
{
    const auto pdu_size = getRawSize();
    auto encodedTlv     = std::vector<uint8_t>{};

    encodedTlv.reserve(pdu_size);

    encodedTlv.push_back(utils::toUnderlying(TLVType::MessageToUser));
    encodedTlv.push_back(message.length());

    std::vector<uint8_t> messageBytes(message.begin(), message.end());

    utils::concatenateVectorsInplace(messageBytes, encodedTlv);

    return encodedTlv;
}

cfdp::pdu::tlv::EntityId::EntityId(std::span<uint8_t const> memory)
{

    const auto memory_size = memory.size();

    if (memory_size < sizeof(uint8_t) + sizeof(uint8_t))
    {
        throw exception::DecodeFromBytesException("Passed memory does not contain enough bytes");
    }

    if (memory[0] != utils::toUnderlying(TLVType::EntityId))
    {
        throw exception::DecodeFromBytesException("TLVType is not Enitity Id");
    }

    lengthOfEntityID = memory[1];

    if (memory_size < sizeof(uint8_t) + sizeof(uint8_t) + lengthOfEntityID)
    {
        throw exception::DecodeFromBytesException("Passed memory does not contain enough bytes");
    }

    faultEntityID = utils::bytesToInt<uint64_t>(memory, 2, lengthOfEntityID);
}

std::vector<uint8_t> cfdp::pdu::tlv::EntityId::encodeToBytes() const
{
    const auto pdu_size = getRawSize();
    auto encodedTlv     = std::vector<uint8_t>{};

    encodedTlv.reserve(pdu_size);

    encodedTlv.push_back(utils::toUnderlying(TLVType::EntityId));
    encodedTlv.push_back(lengthOfEntityID);

    auto faultEntityIDBytes = utils::intToBytes(faultEntityID, lengthOfEntityID);
    utils::concatenateVectorsInplace(faultEntityIDBytes, encodedTlv);

    return encodedTlv;
}
