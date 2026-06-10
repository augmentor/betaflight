#include "msp/serial_port.h"
#include "msp/msp_exception.h"
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  include <setupapi.h>
#  pragma comment(lib, "setupapi.lib")
#elif defined(__APPLE__)
#  include <IOKit/IOKitLib.h>
#  include <IOKit/serial/IOSerialKeys.h>
#  include <CoreFoundation/CoreFoundation.h>
#  include <fcntl.h>
#  include <unistd.h>
#  include <termios.h>
#  include <sys/ioctl.h>
#  include <poll.h>
#else
// Linux
#  include <fcntl.h>
#  include <unistd.h>
#  include <termios.h>
#  include <poll.h>
#  include <dirent.h>
#  include <sys/stat.h>
#  include <cstdio>
#endif

namespace msp {

// ── listPorts ─────────────────────────────────────────────

#ifdef _WIN32

std::vector<PortInfo> SerialPort::listPorts() {
    std::vector<PortInfo> result;
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      R"(HARDWARE\DEVICEMAP\SERIALCOMM)",
                      0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return result;

    char valueName[256], data[256];
    DWORD idx = 0;
    while (true) {
        DWORD vnLen = sizeof(valueName), dataLen = sizeof(data),
              type = REG_SZ;
        LONG ret = RegEnumValueA(hKey, idx++, valueName, &vnLen,
                                 nullptr, &type,
                                 reinterpret_cast<BYTE*>(data), &dataLen);
        if (ret == ERROR_NO_MORE_ITEMS) break;
        if (ret != ERROR_SUCCESS)       break;
        PortInfo pi;
        pi.port        = data;
        pi.description = valueName;
        result.push_back(pi);
    }
    RegCloseKey(hKey);
    return result;
}

#elif defined(__APPLE__)

std::vector<PortInfo> SerialPort::listPorts() {
    std::vector<PortInfo> result;
    CFMutableDictionaryRef matchDict =
        IOServiceMatching(kIOSerialBSDServiceValue);
    if (!matchDict) return result;

    CFDictionarySetValue(matchDict,
                         CFSTR(kIOSerialBSDTypeKey),
                         CFSTR(kIOSerialBSDAllTypes));

    io_iterator_t iter = 0;
    kern_return_t kr = IOServiceGetMatchingServices(
        kIOMainPortDefault, matchDict, &iter);
    if (kr != KERN_SUCCESS) return result;

    io_service_t service;
    while ((service = IOIteratorNext(iter)) != 0) {
        CFStringRef path = (CFStringRef)IORegistryEntryCreateCFProperty(
            service, CFSTR(kIOCalloutDeviceKey),
            kCFAllocatorDefault, 0);
        if (path) {
            char buf[256];
            if (CFStringGetCString(path, buf, sizeof(buf),
                                   kCFStringEncodingUTF8)) {
                PortInfo pi;
                pi.port = buf;
                result.push_back(pi);
            }
            CFRelease(path);
        }
        IOObjectRelease(service);
    }
    IOObjectRelease(iter);
    return result;
}

#else // Linux

std::vector<PortInfo> SerialPort::listPorts() {
    std::vector<PortInfo> result;
    const char* sysPath = "/sys/class/tty";
    DIR* dir = opendir(sysPath);
    if (!dir) return result;

    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        if (ent->d_name[0] == '.') continue;

        // Only include entries that have a device symlink
        char devPath[512];
        snprintf(devPath, sizeof(devPath), "%s/%s/device", sysPath, ent->d_name);
        struct stat st;
        if (stat(devPath, &st) != 0) continue;

        PortInfo pi;
        pi.port = std::string("/dev/") + ent->d_name;
        // Try to read driver name from uevent
        char ueventPath[512];
        snprintf(ueventPath, sizeof(ueventPath),
                 "%s/%s/device/uevent", sysPath, ent->d_name);
        FILE* f = fopen(ueventPath, "r");
        if (f) {
            char line[256];
            while (fgets(line, sizeof(line), f)) {
                if (strncmp(line, "DRIVER=", 7) == 0) {
                    pi.description = std::string(line + 7);
                    if (!pi.description.empty() &&
                        pi.description.back() == '\n')
                        pi.description.pop_back();
                    break;
                }
            }
            fclose(f);
        }
        result.push_back(pi);
    }
    closedir(dir);
    return result;
}

#endif

// ── Platform baud helpers ─────────────────────────────────

#ifndef _WIN32
static speed_t toBaudRate(int baud) {
    switch (baud) {
    case 9600:   return B9600;
    case 19200:  return B19200;
    case 38400:  return B38400;
    case 57600:  return B57600;
    case 115200: return B115200;
    case 230400: return B230400;
#ifdef B460800
    case 460800: return B460800;
#endif
#ifdef B921600
    case 921600: return B921600;
#endif
    default:     return B115200;
    }
}
#endif

// ── open / close / isOpen ─────────────────────────────────

#ifdef _WIN32

bool SerialPort::open(const std::string& port, int baudrate) {
    std::string p = (port.rfind("\\\\.\\", 0) == 0) ? port : "\\\\.\\" + port;
    HANDLE h = CreateFileA(p.c_str(),
                           GENERIC_READ | GENERIC_WRITE,
                           0, nullptr, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        throw MspException(MspErrorCode::SerialError,
                           "Cannot open port: " + port);
    handle_ = h;

    DCB dcb{};
    dcb.DCBlength = sizeof(dcb);
    GetCommState(h, &dcb);
    dcb.BaudRate = static_cast<DWORD>(baudrate);
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    SetCommState(h, &dcb);

    COMMTIMEOUTS to{};
    to.ReadIntervalTimeout         = MAXDWORD;
    to.ReadTotalTimeoutMultiplier  = MAXDWORD;
    to.ReadTotalTimeoutConstant    = 500; // default 500ms; overridden per-read
    to.WriteTotalTimeoutConstant   = 2000;
    to.WriteTotalTimeoutMultiplier = 10;
    SetCommTimeouts(h, &to);
    return true;
}

void SerialPort::close() {
    if (isOpen()) {
        CloseHandle(static_cast<HANDLE>(handle_));
        handle_ = INVALID_HANDLE_VALUE;
    }
}

bool SerialPort::isOpen() const {
    return handle_ != INVALID_HANDLE_VALUE && handle_ != nullptr;
}

int SerialPort::write(const uint8_t* data, int len) {
    if (!isOpen()) return -1;
    DWORD written = 0;
    if (!WriteFile(static_cast<HANDLE>(handle_), data,
                   static_cast<DWORD>(len), &written, nullptr))
        return -1;
    return static_cast<int>(written);
}

int SerialPort::read(uint8_t* buf, int maxLen, int timeoutMs) {
    if (!isOpen()) return -1;
    HANDLE h = static_cast<HANDLE>(handle_);

    COMMTIMEOUTS to{};
    to.ReadIntervalTimeout         = MAXDWORD;
    to.ReadTotalTimeoutMultiplier  = MAXDWORD;
    to.ReadTotalTimeoutConstant    = static_cast<DWORD>(timeoutMs);
    to.WriteTotalTimeoutConstant   = 2000;
    to.WriteTotalTimeoutMultiplier = 10;
    SetCommTimeouts(h, &to);

    DWORD nRead = 0;
    if (!ReadFile(h, buf, static_cast<DWORD>(maxLen), &nRead, nullptr))
        return -1;
    return static_cast<int>(nRead);
}

void SerialPort::flush() {
    if (isOpen()) PurgeComm(static_cast<HANDLE>(handle_),
                             PURGE_RXCLEAR | PURGE_TXCLEAR);
}

#else // POSIX (Linux + macOS)

bool SerialPort::open(const std::string& port, int baudrate) {
    int fd = ::open(port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0)
        throw MspException(MspErrorCode::SerialError,
                           "Cannot open port: " + port);
    fd_ = fd;

    // Remove O_NONBLOCK once opened (we use poll for timeouts)
    int flags = fcntl(fd_, F_GETFL, 0);
    fcntl(fd_, F_SETFL, flags & ~O_NONBLOCK);

    struct termios tty{};
    tcgetattr(fd_, &tty);

    speed_t spd = toBaudRate(baudrate);
    cfsetispeed(&tty, spd);
    cfsetospeed(&tty, spd);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | CSTOPB | CRTSCTS);
    tty.c_iflag  = IGNPAR;
    tty.c_oflag  = 0;
    tty.c_lflag  = 0;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;

    tcsetattr(fd_, TCSANOW, &tty);
    tcflush(fd_, TCIFLUSH);
    return true;
}

void SerialPort::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

bool SerialPort::isOpen() const { return fd_ >= 0; }

int SerialPort::write(const uint8_t* data, int len) {
    if (!isOpen()) return -1;
    return static_cast<int>(::write(fd_, data, len));
}

int SerialPort::read(uint8_t* buf, int maxLen, int timeoutMs) {
    if (!isOpen()) return -1;
    struct pollfd pfd{ fd_, POLLIN, 0 };
    int ret = poll(&pfd, 1, timeoutMs);
    if (ret <= 0) return (ret == 0) ? 0 : -1;
    ssize_t n = ::read(fd_, buf, static_cast<size_t>(maxLen));
    return static_cast<int>(n);
}

void SerialPort::flush() {
    if (isOpen()) tcflush(fd_, TCIFLUSH);
}

#endif

} // namespace msp
