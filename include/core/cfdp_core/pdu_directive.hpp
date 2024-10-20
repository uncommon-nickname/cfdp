#include "pdu_enums.hpp"
#include "pdu_interface.hpp"

#include <cstdint>
#include <span>

using ::cfdp::pdu::header::LargeFileFlag;

namespace cfdp::pdu::directive
{

class KeepAlive : PduInterface
{
  public:
    KeepAlive(uint32_t progress) : progress(progress), largeFileFlag(LargeFileFlag::SmallFile){};
    KeepAlive(uint64_t progress) : progress(progress), largeFileFlag(LargeFileFlag::LargeFile){};
    KeepAlive(std::span<uint8_t const> memory);

    [[nodiscard]] std::vector<uint8_t> encodeToBytes() const override;

    [[nodiscard]] inline uint16_t getRawSize() const override
    {
        return (largeFileFlag == LargeFileFlag::LargeFile) ? const_large_file_size_bytes
                                                           : const_small_file_size_bytes;
    };

    [[nodiscard]] auto getProgress() const { return progress; }
    [[nodiscard]] auto getLargeFileFlag() const { return largeFileFlag; }

  private:
    uint64_t progress;
    LargeFileFlag largeFileFlag;

    static constexpr uint16_t const_small_file_size_bytes = sizeof(uint8_t) + sizeof(uint32_t);
    static constexpr uint16_t const_large_file_size_bytes = sizeof(uint8_t) + sizeof(uint64_t);

    [[nodiscard]] inline uint8_t getProgressSize() const
    {
        return (largeFileFlag == LargeFileFlag::LargeFile) ? sizeof(uint64_t) : sizeof(uint32_t);
    }
};

class Ack : PduInterface
{
  public:
    Ack(Directive directiveCode, Condition conditionCode, TransactionStatus transactionStatus);
    Ack(std::span<uint8_t const> memory);

    [[nodiscard]] std::vector<uint8_t> encodeToBytes() const override;
    [[nodiscard]] inline uint16_t getRawSize() const override { return const_pdu_size_bytes; };

    [[nodiscard]] auto getDirectiveCode() const { return directiveCode; }
    [[nodiscard]] auto getConditionCode() const { return conditionCode; }
    [[nodiscard]] auto getTransactionStatus() const { return transactionStatus; }

  private:
    Directive directiveCode;
    DirectiveSubtype directiveSubtype;
    Condition conditionCode;
    TransactionStatus transactionStatus;

    static constexpr uint16_t const_pdu_size_bytes = sizeof(uint8_t) + sizeof(uint16_t);
};

template <class T>
class EndOfFile : PduInterface
{
  public:
    EndOfFile(Condition conditionCode, uint32_t checksum, T fileSize, uint8_t lengthOfEntityID,
              uint64_t faultEntityID);
    EndOfFile(Condition conditionCode, uint32_t checksum, T fileSize);
    EndOfFile(std::span<uint8_t const> memory);

    [[nodiscard]] std::vector<uint8_t> encodeToBytes() const override;

    [[nodiscard]] inline uint16_t getRawSize() const override
    {
        return const_pdu_size_bytes + getSizeOfFileSize() + getFaultLocationSize();
    };

    [[nodiscard]] auto getConditionCode() const { return conditionCode; }
    [[nodiscard]] auto getFileSize() const { return fileSize; }
    [[nodiscard]] auto getChecksum() const { return checksum; }
    [[nodiscard]] auto getLengthOfEntityID() const { return lengthOfEntityID; }
    [[nodiscard]] auto getFaultEntityID() const { return faultEntityID; }

  private:
    T fileSize;
    Condition conditionCode;
    uint32_t checksum;
    uint8_t lengthOfEntityID = 0;
    uint64_t faultEntityID   = 0;

    static constexpr uint8_t const_pdu_size_bytes =
        sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(fileSize);

    [[nodiscard]] inline bool isError() const { return conditionCode != Condition::NoError; }
    [[nodiscard]] inline uint8_t getSizeOfFileSize() const { return sizeof(fileSize); }
    [[nodiscard]] inline uint8_t getFaultLocationSize() const
    {
        return (isError()) ? sizeof(uint8_t) + sizeof(uint8_t) + lengthOfEntityID : 0;
    }
};

} // namespace cfdp::pdu::directive
