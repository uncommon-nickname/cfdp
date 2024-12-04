#pragma once

#include "cfdp_core/pdu_enums.hpp"
#include "cfdp_core/pdu_interface.hpp"
#include <cstdint>
#include <optional>
#include <span>
#include <string>

namespace cfdp::pdu::tlv
{
class FilestoreRequest : PduInterface
{
  public:
    FilestoreRequest(FilestoreRequestActionCode actionCode, std::string firstFileName);
    FilestoreRequest(FilestoreRequestActionCode actionCode, std::string firstFileName,
                     std::string secondFileName);
    FilestoreRequest(std::span<uint8_t const> memory);

    [[nodiscard]] std::vector<uint8_t> encodeToBytes() const override;

    [[nodiscard]] inline uint16_t getRawSize() const override
    {
        return sizeof(uint8_t) + sizeof(uint8_t) + valueSize();
    };

    FilestoreRequestActionCode actionCode;
    std::string firstFileName;
    std::optional<std::string> secondFileName;

  private:
    [[nodiscard]] inline bool shouldHaveSecondFile() const
    {
        return actionCode == FilestoreRequestActionCode::RenameFile ||
               actionCode == FilestoreRequestActionCode::AppendFile ||
               actionCode == FilestoreRequestActionCode::ReplaceFile;
    }

    [[nodiscard]] inline uint8_t valueSize() const
    {
        return sizeof(uint8_t) + firstFileSize() + secondFileSize();
    }

    [[nodiscard]] inline uint8_t firstFileSize() const
    {
        return sizeof(uint8_t) + firstFileName.length();
    }

    [[nodiscard]] inline uint8_t secondFileSize() const
    {
        return shouldHaveSecondFile() ? sizeof(uint8_t) + secondFileName->length() : 0;
    }
};
class EntityId : PduInterface
{
  public:
    EntityId(uint8_t lengthOfEntityID, uint64_t faultEntityID)
        : lengthOfEntityID(lengthOfEntityID), faultEntityID(faultEntityID){};
    EntityId(std::span<uint8_t const> memory);

    [[nodiscard]] std::vector<uint8_t> encodeToBytes() const override;

    [[nodiscard]] inline uint16_t getRawSize() const override
    {
        return sizeof(uint8_t) + sizeof(uint8_t) + lengthOfEntityID;
    };

    uint8_t lengthOfEntityID;
    uint64_t faultEntityID;
};
} // namespace cfdp::pdu::tlv
