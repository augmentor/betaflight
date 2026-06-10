#pragma once
#include <cstdint>
#include <vector>
#include "msp_types.h"

namespace msp {

struct MspMessage {
    MspVersion   version   = MspVersion::V1;
    MspDirection direction = MspDirection::Request;
    uint16_t     command   = 0;
    std::vector<uint8_t> payload;
    uint8_t      flag      = 0;  // v2 only
};

class MspEncoder {
public:
    static std::vector<uint8_t> encode(const MspMessage& msg);

private:
    static uint8_t crcV1(uint8_t size, uint8_t cmd, const uint8_t* payload, int len);
    static uint8_t crc8DvbS2(const uint8_t* data, int len);
    static uint8_t crc8DvbS2Byte(uint8_t crc, uint8_t byte);
};

class MspDecoder {
public:
    bool push(uint8_t byte);
    const MspMessage& message() const { return msg_; }
    void reset();

private:
    enum class State {
        IDLE,
        PREAMBLE_M_OR_X,   // got '$'
        HEADER_DIR,        // got 'M' or 'X' — next byte is direction
        // V1 states
        V1_SIZE,
        V1_CMD,
        V1_PAYLOAD,
        V1_CRC,
        // V2 states (native $X)
        V2_FLAG,
        V2_CMD_LO,
        V2_CMD_HI,
        V2_SIZE_LO,
        V2_SIZE_HI,
        V2_PAYLOAD,
        V2_CRC,
    };

    State    state_      = State::IDLE;
    bool     isV2Native_ = false;
    MspMessage msg_;
    uint16_t payloadIdx_ = 0;
    uint16_t payloadLen_ = 0;
    uint8_t  crc1_       = 0;  // V1 XOR running checksum
    uint8_t  crc2_       = 0;  // V2 DVB-S2 running CRC

    static uint8_t crc8DvbS2Byte(uint8_t crc, uint8_t byte);
};

// ── Typed payload helpers ─────────────────────────────────

namespace codec {

// Little-endian read helpers
inline uint16_t readU16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}
inline uint32_t readU32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0])
         | (static_cast<uint32_t>(p[1]) << 8)
         | (static_cast<uint32_t>(p[2]) << 16)
         | (static_cast<uint32_t>(p[3]) << 24);
}
inline int16_t readS16(const uint8_t* p) {
    return static_cast<int16_t>(readU16(p));
}
inline int32_t readS32(const uint8_t* p) {
    return static_cast<int32_t>(readU32(p));
}

inline void writeU8(std::vector<uint8_t>& v, uint8_t x)  { v.push_back(x); }
inline void writeU16(std::vector<uint8_t>& v, uint16_t x) {
    v.push_back(x & 0xFF);
    v.push_back((x >> 8) & 0xFF);
}
inline void writeU32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(x & 0xFF);
    v.push_back((x >> 8) & 0xFF);
    v.push_back((x >> 16) & 0xFF);
    v.push_back((x >> 24) & 0xFF);
}
inline void writeS16(std::vector<uint8_t>& v, int16_t x)  { writeU16(v, static_cast<uint16_t>(x)); }

} // namespace codec

} // namespace msp
