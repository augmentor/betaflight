#include "msp/msp_codec.h"
#include <stdexcept>

namespace msp {

// ── CRC helpers ───────────────────────────────────────────

uint8_t MspEncoder::crcV1(uint8_t size, uint8_t cmd, const uint8_t* payload, int len) {
    uint8_t crc = size ^ cmd;
    for (int i = 0; i < len; ++i) crc ^= payload[i];
    return crc;
}

uint8_t MspEncoder::crc8DvbS2Byte(uint8_t crc, uint8_t byte) {
    crc ^= byte;
    for (int i = 0; i < 8; ++i)
        crc = (crc & 0x80) ? (crc << 1) ^ 0xD5 : (crc << 1);
    return crc;
}

uint8_t MspEncoder::crc8DvbS2(const uint8_t* data, int len) {
    uint8_t crc = 0;
    for (int i = 0; i < len; ++i) crc = crc8DvbS2Byte(crc, data[i]);
    return crc;
}

// ── Encoder ───────────────────────────────────────────────

std::vector<uint8_t> MspEncoder::encode(const MspMessage& msg) {
    const uint8_t dir = static_cast<uint8_t>(msg.direction);
    const bool forceV2 = (msg.command > 255) || (msg.version == MspVersion::V2);

    if (forceV2) {
        // $X<  flag  cmdLo cmdHi  sizeLo sizeHi  payload  crc
        std::vector<uint8_t> out;
        out.reserve(9 + msg.payload.size());
        out.push_back('$');
        out.push_back('X');
        out.push_back(dir);
        out.push_back(msg.flag);
        out.push_back(msg.command & 0xFF);
        out.push_back((msg.command >> 8) & 0xFF);
        uint16_t sz = static_cast<uint16_t>(msg.payload.size());
        out.push_back(sz & 0xFF);
        out.push_back((sz >> 8) & 0xFF);
        for (uint8_t b : msg.payload) out.push_back(b);

        // CRC covers everything from flag onward (index 3..end)
        uint8_t crc = 0;
        for (size_t i = 3; i < out.size(); ++i)
            crc = crc8DvbS2Byte(crc, out[i]);
        out.push_back(crc);
        return out;
    } else {
        // $M<  size  cmd  payload  crc
        const uint8_t sz  = static_cast<uint8_t>(msg.payload.size());
        const uint8_t cmd = static_cast<uint8_t>(msg.command);
        std::vector<uint8_t> out;
        out.reserve(6 + msg.payload.size());
        out.push_back('$');
        out.push_back('M');
        out.push_back(dir);
        out.push_back(sz);
        out.push_back(cmd);
        for (uint8_t b : msg.payload) out.push_back(b);
        out.push_back(crcV1(sz, cmd, msg.payload.data(),
                            static_cast<int>(msg.payload.size())));
        return out;
    }
}

// ── Decoder ───────────────────────────────────────────────

uint8_t MspDecoder::crc8DvbS2Byte(uint8_t crc, uint8_t byte) {
    crc ^= byte;
    for (int i = 0; i < 8; ++i)
        crc = (crc & 0x80) ? (crc << 1) ^ 0xD5 : (crc << 1);
    return crc;
}

void MspDecoder::reset() {
    state_      = State::IDLE;
    isV2Native_ = false;
    msg_        = MspMessage{};
    payloadIdx_ = 0;
    payloadLen_ = 0;
    crc1_       = 0;
    crc2_       = 0;
}

bool MspDecoder::push(uint8_t b) {
    switch (state_) {
    case State::IDLE:
        if (b == '$') state_ = State::PREAMBLE_M_OR_X;
        break;

    case State::PREAMBLE_M_OR_X:
        if      (b == 'M') { isV2Native_ = false; state_ = State::HEADER_DIR; }
        else if (b == 'X') { isV2Native_ = true;  state_ = State::HEADER_DIR; }
        else               { state_ = State::IDLE; }
        break;

    case State::HEADER_DIR:
        if (b == '<' || b == '>' || b == '!') {
            msg_.direction = static_cast<MspDirection>(b);
            if (isV2Native_) {
                msg_.version = MspVersion::V2;
                crc2_ = 0;
                state_ = State::V2_FLAG;
            } else {
                msg_.version = MspVersion::V1;
                crc1_ = 0;
                state_ = State::V1_SIZE;
            }
        } else {
            state_ = State::IDLE;
        }
        break;

    // ── V1 ────────────────────────────────────────────────
    case State::V1_SIZE:
        payloadLen_ = b;
        crc1_ = b;
        state_ = State::V1_CMD;
        break;

    case State::V1_CMD:
        msg_.command = b;
        crc1_ ^= b;
        msg_.payload.clear();
        msg_.payload.reserve(payloadLen_);
        payloadIdx_ = 0;
        state_ = (payloadLen_ > 0) ? State::V1_PAYLOAD : State::V1_CRC;
        break;

    case State::V1_PAYLOAD:
        msg_.payload.push_back(b);
        crc1_ ^= b;
        if (++payloadIdx_ == payloadLen_) state_ = State::V1_CRC;
        break;

    case State::V1_CRC:
        if (b == crc1_) { state_ = State::IDLE; return true; }
        reset();
        break;

    // ── V2 native ─────────────────────────────────────────
    case State::V2_FLAG:
        msg_.flag = b;
        crc2_ = crc8DvbS2Byte(crc2_, b);
        state_ = State::V2_CMD_LO;
        break;

    case State::V2_CMD_LO:
        msg_.command = b;
        crc2_ = crc8DvbS2Byte(crc2_, b);
        state_ = State::V2_CMD_HI;
        break;

    case State::V2_CMD_HI:
        msg_.command |= static_cast<uint16_t>(b) << 8;
        crc2_ = crc8DvbS2Byte(crc2_, b);
        state_ = State::V2_SIZE_LO;
        break;

    case State::V2_SIZE_LO:
        payloadLen_ = b;
        crc2_ = crc8DvbS2Byte(crc2_, b);
        state_ = State::V2_SIZE_HI;
        break;

    case State::V2_SIZE_HI:
        payloadLen_ |= static_cast<uint16_t>(b) << 8;
        crc2_ = crc8DvbS2Byte(crc2_, b);
        msg_.payload.clear();
        msg_.payload.reserve(payloadLen_);
        payloadIdx_ = 0;
        state_ = (payloadLen_ > 0) ? State::V2_PAYLOAD : State::V2_CRC;
        break;

    case State::V2_PAYLOAD:
        msg_.payload.push_back(b);
        crc2_ = crc8DvbS2Byte(crc2_, b);
        if (++payloadIdx_ == payloadLen_) state_ = State::V2_CRC;
        break;

    case State::V2_CRC:
        if (b == crc2_) { state_ = State::IDLE; return true; }
        reset();
        break;
    }
    return false;
}

} // namespace msp
