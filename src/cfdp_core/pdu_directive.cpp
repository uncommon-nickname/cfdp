#include <cfdp_core/pdu_directive.hpp>
#include <cfdp_core/pdu_enums.hpp>
#include <cfdp_core/pdu_exceptions.hpp>
#include <cfdp_core/utils.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace
{
constexpr uint8_t ack_directive_code_bitmask         = 0b1111'0000;
constexpr uint8_t ack_directive_subtype_code_bitmask = 0b0000'1111;
constexpr uint8_t ack_condition_code_bitmask         = 0b1111'0000;
constexpr uint8_t ack_transaction_status_bitmask     = 0b0000'0011;

constexpr uint8_t eof_condition_code_bitmask = 0b1111'0000;
} // namespace

namespace header    = ::cfdp::pdu::header;
namespace utils     = ::cfdp::utils;
namespace exception = ::cfdp::pdu::exception;

cfdp::pdu::directive::KeepAlivePdu::KeepAlivePdu(std::span<uint8_t const> memory)
{
    const auto memory_size = memory.size();

    if (memory_size < const_small_file_size_bytes)
    {
        throw exception::DecodeFromBytesException("Passed memory does not contain enough bytes");
    }

    if (memory[0] != utils::toUnderlying(Directive::KeepAlive))
    {
        throw exception::DecodeFromBytesException("File Directive code is not Keep Alive Pdu");
    }

    largeFileFlag = (memory_size > const_small_file_size_bytes) ? header::LargeFileFlag::LargeFile
                                                                : header::LargeFileFlag::SmallFile;
    progress      = utils::bytesToInt<uint64_t>(memory, 1, getProgressSize());
};

std::vector<uint8_t> cfdp::pdu::directive::KeepAlivePdu::encodeToBytes() const
{
    const auto pdu_size = getRawSize();
    auto encodedPdu     = std::vector<uint8_t>{};

    encodedPdu.reserve(pdu_size);

    encodedPdu.push_back(utils::toUnderlying(Directive::KeepAlive));

    auto progressBytes = utils::intToBytes(progress, getProgressSize());
    utils::concatenateVectorsInplace(progressBytes, encodedPdu);

    return encodedPdu;
}

cfdp::pdu::directive::AckPdu::AckPdu(Directive directiveCode, Condition conditionCode,
                                     TransactionStatus transactionStatus)
    : directiveCode(directiveCode), conditionCode(conditionCode),
      transactionStatus(transactionStatus)
{
    if (directiveCode != Directive::Eof and directiveCode != Directive::Finished)
    {
        throw exception::PduConstructionException("Only EOF and Finished PDUs can be acknowledged");
    }

    directiveSubtype =
        (directiveCode == Directive::Eof) ? DirectiveSubtype::Eof : DirectiveSubtype::Finished;
}

cfdp::pdu::directive::AckPdu::AckPdu(std::span<uint8_t const> memory)
{
    if (memory.size() != const_pdu_size_bytes)
    {
        throw exception::DecodeFromBytesException("Passed memory has invalid size");
    }

    if (memory[0] != utils::toUnderlying(Directive::Ack))
    {
        throw exception::DecodeFromBytesException("File Directive code is not Ack Pdu");
    }

    const auto secondByte = memory[1];

    directiveCode    = Directive((secondByte & ack_directive_code_bitmask) >> 4);
    directiveSubtype = DirectiveSubtype((secondByte & ack_directive_subtype_code_bitmask));

    const auto thirdByte = memory[2];

    conditionCode     = Condition((thirdByte & ack_condition_code_bitmask) >> 4);
    transactionStatus = TransactionStatus((thirdByte & ack_transaction_status_bitmask));
}

std::vector<uint8_t> cfdp::pdu::directive::AckPdu::encodeToBytes() const
{
    const auto pdu_size = getRawSize();
    auto encodedPdu     = std::vector<uint8_t>{};

    encodedPdu.reserve(pdu_size);

    encodedPdu.push_back(utils::toUnderlying(Directive::Ack));

    const uint8_t secondByte =
        (utils::toUnderlying(directiveCode) << 4) | (utils::toUnderlying(directiveSubtype));

    encodedPdu.push_back(secondByte);

    const uint8_t thirdByte =
        (utils::toUnderlying(conditionCode) << 4) | (utils::toUnderlying(transactionStatus));

    encodedPdu.push_back(thirdByte);

    return encodedPdu;
}

cfdp::pdu::directive::EndOfFile::EndOfFile(Condition conditionCode, uint32_t checksum,
                                           uint32_t fileSize)
    : conditionCode(conditionCode), checksum(checksum), fileSize(fileSize),
      largeFileFlag(header::LargeFileFlag::SmallFile), lengthOfEntityID(0), faultEntityID(0)
{
    if (conditionCode != Condition::NoError)
    {
        throw exception::PduConstructionException(
            "Fault location cannot be omitted with `No error` condition code");
    }
};

cfdp::pdu::directive::EndOfFile::EndOfFile(Condition conditionCode, uint32_t checksum,
                                           uint64_t fileSize)
    : conditionCode(conditionCode), checksum(checksum), fileSize(fileSize),
      largeFileFlag(header::LargeFileFlag::LargeFile), lengthOfEntityID(0), faultEntityID(0)
{
    if (conditionCode != Condition::NoError)
    {
        throw exception::PduConstructionException(
            "Fault location cannot be omitted with `No error` condition code");
    }
};

cfdp::pdu::directive::EndOfFile::EndOfFile(Condition conditionCode, uint32_t checksum,
                                           uint32_t fileSize, uint8_t lengthOfEntityID,
                                           uint64_t faultEntityID)
    : conditionCode(conditionCode), checksum(checksum), fileSize(fileSize),
      largeFileFlag(header::LargeFileFlag::SmallFile), lengthOfEntityID(lengthOfEntityID),
      faultEntityID(faultEntityID)
{
    if (conditionCode == Condition::NoError)
    {
        throw exception::PduConstructionException(
            "Fault location should be omitted with `No error` condition code");
    }
};

cfdp::pdu::directive::EndOfFile::EndOfFile(Condition conditionCode, uint32_t checksum,
                                           uint64_t fileSize, uint8_t lengthOfEntityID,
                                           uint64_t faultEntityID)
    : conditionCode(conditionCode), checksum(checksum), fileSize(fileSize),
      largeFileFlag(header::LargeFileFlag::LargeFile), lengthOfEntityID(lengthOfEntityID),
      faultEntityID(faultEntityID)
{
    if (conditionCode == Condition::NoError)
    {
        throw exception::PduConstructionException(
            "Fault location should be omitted with `No error` condition code");
    }
};

cfdp::pdu::directive::EndOfFile::EndOfFile(std::span<uint8_t const> memory,
                                           LargeFileFlag largeFileFlag)
    : largeFileFlag(largeFileFlag)
{
    const auto memory_size = memory.size();

    if (memory_size < (largeFileFlag == LargeFileFlag::LargeFile) ? const_large_file_pdu_size_bytes
                                                                  : const_small_file_pdu_size_bytes)
    {
        throw exception::DecodeFromBytesException("Passed memory does not contain enough bytes");
    }

    if (memory[0] != utils::toUnderlying(Directive::Eof))
    {
        throw exception::DecodeFromBytesException("File Directive code is not End of File Pdu");
    }
    const auto secondByte = memory[1];

    conditionCode = Condition((secondByte & eof_condition_code_bitmask) >> 4);
    checksum      = utils::bytesToInt<uint64_t>(memory, 2, sizeof(uint32_t));
    fileSize      = utils::bytesToInt<uint64_t>(memory, 6, getFileSize());

    if (conditionCode == Condition::NoError)
    {
        lengthOfEntityID = 0;
        faultEntityID    = 0;
    }
    else
    {
        lengthOfEntityID = memory[8];
        faultEntityID    = utils::bytesToInt<uint64_t>(memory, 9, lengthOfEntityID);
    }
};

std::vector<uint8_t> cfdp::pdu::directive::EndOfFile::encodeToBytes() const
{
    const auto pdu_size = getRawSize();
    auto encodedPdu     = std::vector<uint8_t>{};

    encodedPdu.reserve(pdu_size);

    encodedPdu.push_back(utils::toUnderlying(Directive::Eof));

    const uint8_t secondByte = (utils::toUnderlying(conditionCode) << 4);

    encodedPdu.push_back(secondByte);

    auto checksumBytes = utils::intToBytes(checksum, sizeof(uint32_t));
    utils::concatenateVectorsInplace(checksumBytes, encodedPdu);

    auto fileSizeBytes = utils::intToBytes(fileSize, getFileSize());
    utils::concatenateVectorsInplace(fileSizeBytes, encodedPdu);

    if (conditionCode != Condition::NoError)
    {
        encodedPdu.push_back(utils::toUnderlying(TLVType::EntityId));
        encodedPdu.push_back(lengthOfEntityID);

        auto faultEntityIDBytes = utils::intToBytes(faultEntityID, lengthOfEntityID);
        utils::concatenateVectorsInplace(fileSizeBytes, faultEntityIDBytes);
    }

    return encodedPdu;
}
