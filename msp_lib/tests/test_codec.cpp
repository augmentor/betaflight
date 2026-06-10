#include <msp/msp_codec.h>
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <vector>
#include <numeric>

using namespace msp;

static int pass = 0, fail = 0;
#define CHECK(expr) do { \
    if (expr) { ++pass; } \
    else { ++fail; std::printf("FAIL: %s  line %d\n", #expr, __LINE__); } \
} while(0)

// Encode a message then decode it byte-by-byte; verify roundtrip.
static MspMessage roundtrip(const MspMessage& in) {
    auto bytes = MspEncoder::encode(in);
    MspDecoder dec;
    for (uint8_t b : bytes) {
        if (dec.push(b)) return dec.message();
    }
    throw std::runtime_error("decoder did not complete");
}

int main() {
    // ── V1 empty payload ──────────────────────────────────
    {
        MspMessage m;
        m.version   = MspVersion::V1;
        m.direction = MspDirection::Request;
        m.command   = static_cast<uint16_t>(MspCommand::MSP_ATTITUDE);
        auto out = MspEncoder::encode(m);
        // $M<  size=0  cmd=108  crc=0^108=108
        CHECK(out.size() == 6);
        CHECK(out[0] == '$');
        CHECK(out[1] == 'M');
        CHECK(out[2] == '<');
        CHECK(out[3] == 0);       // size
        CHECK(out[4] == 108);     // cmd
        CHECK(out[5] == 108);     // crc = 0^108

        auto r = roundtrip(m);
        CHECK(r.version   == MspVersion::V1);
        CHECK(r.direction == MspDirection::Request);
        CHECK(r.command   == 108);
        CHECK(r.payload.empty());
    }

    // ── V1 with payload ───────────────────────────────────
    {
        MspMessage m;
        m.version   = MspVersion::V1;
        m.direction = MspDirection::Response;
        m.command   = static_cast<uint16_t>(MspCommand::MSP_ATTITUDE);
        m.payload   = {0x0A, 0x00, 0x14, 0x00, 0x5A, 0x00};
        auto r = roundtrip(m);
        CHECK(r.command   == static_cast<uint16_t>(MspCommand::MSP_ATTITUDE));
        CHECK(r.direction == MspDirection::Response);
        CHECK(r.payload   == m.payload);
    }

    // ── V1 large payload (254 bytes max for v1) ───────────
    {
        MspMessage m;
        m.version   = MspVersion::V1;
        m.direction = MspDirection::Response;
        m.command   = 200;
        m.payload.resize(254);
        std::iota(m.payload.begin(), m.payload.end(), 0u);
        auto r = roundtrip(m);
        CHECK(r.payload == m.payload);
    }

    // ── V2 empty payload ──────────────────────────────────
    {
        MspMessage m;
        m.version   = MspVersion::V2;
        m.direction = MspDirection::Request;
        m.command   = static_cast<uint16_t>(MspCommand::MSP2_MCU_INFO);
        m.flag      = 0;
        auto out = MspEncoder::encode(m);
        // $X<  flag cmdLo cmdHi sizeLo sizeHi  crc
        CHECK(out.size() == 9);
        CHECK(out[0] == '$');
        CHECK(out[1] == 'X');
        CHECK(out[2] == '<');

        auto r = roundtrip(m);
        CHECK(r.version   == MspVersion::V2);
        CHECK(r.direction == MspDirection::Request);
        CHECK(r.command   == static_cast<uint16_t>(MspCommand::MSP2_MCU_INFO));
        CHECK(r.payload.empty());
    }

    // ── V2 with payload ───────────────────────────────────
    {
        MspMessage m;
        m.version   = MspVersion::V2;
        m.direction = MspDirection::Response;
        m.command   = 0x300C;
        m.payload   = {0x01,0x02,0x03,0x04,0x05};
        auto r = roundtrip(m);
        CHECK(r.command == 0x300C);
        CHECK(r.payload == m.payload);
    }

    // ── Auto-select V2 when cmd > 255 ─────────────────────
    {
        MspMessage m;
        m.version   = MspVersion::V1;  // request V1 but cmd forces V2
        m.direction = MspDirection::Request;
        m.command   = 0x1009;
        auto out = MspEncoder::encode(m);
        CHECK(out[1] == 'X');  // must be V2 frame
    }

    // ── Corrupted CRC is rejected ─────────────────────────
    {
        MspMessage m;
        m.version   = MspVersion::V1;
        m.direction = MspDirection::Request;
        m.command   = 108;
        auto bytes = MspEncoder::encode(m);
        bytes.back() ^= 0xFF;  // corrupt CRC
        MspDecoder dec;
        bool got = false;
        for (uint8_t b : bytes) got = dec.push(b);
        CHECK(!got);  // should be rejected
    }

    // ── Stream: two messages back-to-back ─────────────────
    {
        MspMessage m1, m2;
        m1.version = MspVersion::V1; m1.direction = MspDirection::Request;
        m1.command = 108;
        m2.version = MspVersion::V1; m2.direction = MspDirection::Response;
        m2.command = 109; m2.payload = {0xAB, 0xCD};

        auto b1 = MspEncoder::encode(m1);
        auto b2 = MspEncoder::encode(m2);
        b1.insert(b1.end(), b2.begin(), b2.end());

        MspDecoder dec;
        int count = 0;
        for (uint8_t b : b1) {
            if (dec.push(b)) {
                ++count;
                if (count == 1) CHECK(dec.message().command == 108);
                if (count == 2) {
                    CHECK(dec.message().command == 109);
                    CHECK(dec.message().payload == m2.payload);
                }
                dec.reset();
            }
        }
        CHECK(count == 2);
    }

    // ── Codec helpers ─────────────────────────────────────
    {
        using namespace codec;
        std::vector<uint8_t> buf;
        writeU16(buf, 0x1234);
        CHECK(buf[0] == 0x34 && buf[1] == 0x12);
        CHECK(readU16(buf.data()) == 0x1234);

        buf.clear();
        writeU32(buf, 0xDEADBEEF);
        CHECK(readU32(buf.data()) == 0xDEADBEEF);

        buf.clear();
        writeS16(buf, -1000);
        CHECK(readS16(buf.data()) == -1000);
    }

    std::printf("Codec tests: %d passed, %d failed\n", pass, fail);
    return fail == 0 ? 0 : 1;
}
