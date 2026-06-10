#pragma once
#include "msp_codec.h"
#include "serial_port.h"
#include "msp_exception.h"
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>

namespace msp {

class MspClient {
public:
    explicit MspClient(SerialPort& port);
    ~MspClient();

    MspMessage request(MspCommand cmd,
                       const std::vector<uint8_t>& payload = {},
                       int timeoutMs = 500);

    bool send(MspCommand cmd,
              const std::vector<uint8_t>& payload = {});

    MspVersion detectVersion();

    void setVersion(MspVersion v)  { version_ = v; }
    void setTimeout(int ms)        { timeoutMs_ = ms; }
    void setRetries(int n)         { retries_ = n; }

    int getTimeoutCount()  const { return timeoutCount_; }
    int getCrcErrorCount() const { return crcErrorCount_; }

private:
    SerialPort& port_;
    MspVersion  version_      = MspVersion::V1;
    int         timeoutMs_    = 500;
    int         retries_      = 2;
    int         timeoutCount_ = 0;
    int         crcErrorCount_= 0;

    MspMessage doRequest(MspCommand cmd,
                         const std::vector<uint8_t>& payload,
                         int timeoutMs);
};

} // namespace msp
