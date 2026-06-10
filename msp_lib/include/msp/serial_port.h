#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace msp {

struct PortInfo {
    std::string port;        // e.g. "COM3" or "/dev/ttyACM0"
    std::string description; // human-readable description
    std::string hwId;        // hardware ID (USB VID/PID if available)
};

class SerialPort {
public:
    SerialPort()  = default;
    ~SerialPort() { close(); }

    SerialPort(const SerialPort&)            = delete;
    SerialPort& operator=(const SerialPort&) = delete;
    SerialPort(SerialPort&&)                 = default;
    SerialPort& operator=(SerialPort&&)      = default;

    static std::vector<PortInfo> listPorts();

    bool open(const std::string& port, int baudrate = 115200);
    void close();
    bool isOpen() const;

    // Returns bytes written, -1 on error
    int write(const uint8_t* data, int len);
    int write(const std::vector<uint8_t>& data) {
        return write(data.data(), static_cast<int>(data.size()));
    }

    // Returns bytes read (0 = timeout, -1 = error)
    int read(uint8_t* buf, int maxLen, int timeoutMs = 500);

    void flush();

private:
#ifdef _WIN32
    void* handle_ = reinterpret_cast<void*>(-1); // INVALID_HANDLE_VALUE
#else
    int fd_ = -1;
#endif
};

} // namespace msp
