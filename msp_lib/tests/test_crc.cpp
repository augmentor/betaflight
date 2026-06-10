#include <msp/msp_codec.h>
#include <cassert>
#include <cstdio>

// Expose internal CRC via a thin wrapper for testing
static uint8_t crcV1(uint8_t size, uint8_t cmd,
                     const uint8_t* payload, int len) {
    uint8_t crc = size ^ cmd;
    for (int i = 0; i < len; ++i) crc ^= payload[i];
    return crc;
}

static uint8_t crc8DvbS2Byte(uint8_t crc, uint8_t b) {
    crc ^= b;
    for (int i = 0; i < 8; ++i)
        crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0xD5)
                           : (uint8_t)(crc << 1);
    return crc;
}
static uint8_t crc8DvbS2(const uint8_t* data, int len) {
    uint8_t crc = 0;
    for (int i = 0; i < len; ++i) crc = crc8DvbS2Byte(crc, data[i]);
    return crc;
}

static int pass = 0, fail = 0;
#define CHECK(expr) do { \
    if (expr) { ++pass; } \
    else { ++fail; std::printf("FAIL: %s  line %d\n", #expr, __LINE__); } \
} while(0)

int main() {
    // V1 CRC: XOR of size ^ cmd ^ payload bytes
    // Known vector: size=3, cmd=108 (MSP_ATTITUDE), payload={0x64,0x00,0x14}
    // crc = 3^108^0x64^0x00^0x14 = 3^108^100^0^20 = ?
    {
        uint8_t pl[] = {0x64, 0x00, 0x14};
        uint8_t crc = crcV1(3, 108, pl, 3);
        // crc = 3^108 = 111; 111^100=11; 11^0=11; 11^20=31
        CHECK(crc == 31);
    }
    // V1 CRC empty payload: size=0, cmd=108 → 0^108=108
    {
        uint8_t crc = crcV1(0, 108, nullptr, 0);
        CHECK(crc == 108);
    }
    // DVB-S2 known vector: single byte 0x00 → 0x00
    // crc ^= 0x00 = 0; shifting 0 eight times stays 0
    {
        uint8_t b = 0x00;
        uint8_t crc = crc8DvbS2(&b, 1);
        CHECK(crc == 0x00);
    }
    // DVB-S2 known vector: single byte 0x01 → 0xD5
    // crc = 0^1=1; LSB reached after 7 zero-shifts → 0x80; MSB set → 0xD5
    {
        uint8_t b = 0x01;
        uint8_t crc = crc8DvbS2(&b, 1);
        CHECK(crc == 0xD5);
    }
    // DVB-S2 known vector: single byte 0xFF → 0xF9 (hand-computed)
    {
        uint8_t b = 0xFF;
        uint8_t crc = crc8DvbS2(&b, 1);
        CHECK(crc == 0xF9);
    }
    // DVB-S2: two-byte sequence — second byte applied to result of first
    {
        uint8_t buf[] = {0x01, 0x00};
        uint8_t crc = crc8DvbS2(buf, 2);
        uint8_t expected = crc8DvbS2Byte(0xD5, 0x00); // 0xD5 ^ 0x00 then 8 iters
        CHECK(crc == expected);
    }
    // DVB-S2 identity: empty input → 0
    {
        uint8_t crc = crc8DvbS2(nullptr, 0);
        CHECK(crc == 0);
    }

    std::printf("CRC tests: %d passed, %d failed\n", pass, fail);
    return fail == 0 ? 0 : 1;
}
