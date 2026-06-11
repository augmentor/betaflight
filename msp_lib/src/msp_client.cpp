#include "msp/msp_client.h"
#include <stdexcept>

namespace msp {

MspClient::MspClient(SerialPort& port) : port_(port) {}
MspClient::~MspClient() = default;

bool MspClient::send(MspCommand cmd, const std::vector<uint8_t>& payload) {
    if (!port_.isOpen())
        throw MspException(MspErrorCode::PortNotOpen, "Serial port is not open");

    MspMessage msg;
    msg.version   = (static_cast<uint16_t>(cmd) > 255)
                    ? MspVersion::V2 : version_;
    msg.direction = MspDirection::Request;
    msg.command   = static_cast<uint16_t>(cmd);
    msg.payload   = payload;

    auto bytes = MspEncoder::encode(msg);
    int written = port_.write(bytes);
    return written == static_cast<int>(bytes.size());
}

MspMessage MspClient::doRequest(MspCommand cmd,
                                 const std::vector<uint8_t>& payload,
                                 int timeoutMs) {
    port_.flush();
    if (!send(cmd, payload))
        throw MspException(MspErrorCode::SerialError, "Write failed");

    MspDecoder dec;
    uint8_t buf[256];
    const uint16_t wantCmd = static_cast<uint16_t>(cmd);

    // Read bytes until we assemble a matching response or time out.
    // We loop consuming chunks from read() until timeout accumulates.
    int remaining = timeoutMs;
    while (remaining > 0) {
        int chunk = (remaining < 50) ? remaining : 50;
        int n = port_.read(buf, sizeof(buf), chunk);
        remaining -= chunk;
        if (n < 0)
            throw MspException(MspErrorCode::SerialError, "Read error");

        for (int i = 0; i < n; ++i) {
            if (dec.push(buf[i])) {
                const MspMessage& m = dec.message();
                if (m.command == wantCmd &&
                    m.direction == MspDirection::Response) {
                    return m;
                }
                if (m.direction == MspDirection::Error) {
                    throw MspException(MspErrorCode::InvalidResponse,
                                       "FC returned error for command");
                }
                dec.reset();
            }
        }
    }

    ++timeoutCount_;
    throw MspException(MspErrorCode::Timeout, "Request timed out");
}

MspMessage MspClient::request(MspCommand cmd,
                               const std::vector<uint8_t>& payload,
                               int timeoutMs) {
    if (!port_.isOpen())
        throw MspException(MspErrorCode::PortNotOpen, "Serial port is not open");

    for (int attempt = 0; attempt <= retries_; ++attempt) {
        try {
            return doRequest(cmd, payload, timeoutMs);
        } catch (const MspException& e) {
            if (e.code() != MspErrorCode::Timeout || attempt == retries_)
                throw;
        }
    }
    // unreachable
    throw MspException(MspErrorCode::Timeout, "Request timed out");
}

MspVersion MspClient::detectVersion() {
    // Try V2 first
    version_ = MspVersion::V2;
    try {
        MspMessage msg;
        msg.version   = MspVersion::V2;
        msg.direction = MspDirection::Request;
        msg.command   = static_cast<uint16_t>(MspCommand::MSP_API_VERSION);
        auto bytes = MspEncoder::encode(msg);
        port_.flush();
        port_.write(bytes);

        MspDecoder dec;
        uint8_t buf[64];
        for (int t = 0; t < 10; ++t) {
            int n = port_.read(buf, sizeof(buf), 100);
            for (int i = 0; i < n; ++i) {
                if (dec.push(buf[i])) {
                    const MspMessage& m = dec.message();
                    if (m.command ==
                        static_cast<uint16_t>(MspCommand::MSP_API_VERSION) &&
                        m.direction == MspDirection::Response &&
                        !m.payload.empty()) {
                        version_ = MspVersion::V2;
                        return version_;
                    }
                    dec.reset();
                }
            }
        }
    } catch (...) {}

    version_ = MspVersion::V1;
    return version_;
}

} // namespace msp
