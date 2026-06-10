#pragma once
#include <stdexcept>
#include <string>

namespace msp {

enum class MspErrorCode {
    Timeout,
    CrcError,
    SerialError,
    InvalidResponse,
    PortNotOpen,
    PayloadTooShort,
};

class MspException : public std::runtime_error {
public:
    MspException(MspErrorCode code, const std::string& msg)
        : std::runtime_error(msg), code_(code) {}

    MspErrorCode code() const noexcept { return code_; }

private:
    MspErrorCode code_;
};

} // namespace msp
