#pragma once
#include "msp_client.h"
#include "msp_types.h"
#include "msp_codec.h"
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>

namespace msp {

class MspCommands {
public:
    explicit MspCommands(MspClient& client);
    ~MspCommands();

    // ── Identification ───────────────────────────────────
    MspApiVersion getApiVersion();
    std::string   getFcVariant();    // "BTFL", "INAV"
    std::string   getFcVersion();    // "4.5.1"
    MspBoardInfo  getBoardInfo();
    MspBuildInfo  getBuildInfo();
    std::string   getName();
    bool          setName(const std::string& name);

    // ── Telemetry ────────────────────────────────────────
    MspAttitude   getAttitude();
    MspRawImu     getRawImu();
    MspStatus     getStatus();
    MspStatusEx   getStatusEx();
    MspMotors     getMotors();
    MspRcChannels getRc();
    MspAnalog     getAnalog();
    MspBatteryState getBatteryState();
    int           getAltitude();    // cm

    // ── PID ──────────────────────────────────────────────
    MspPid          getPid();
    bool            setPid(const MspPid& pid);
    MspRcTuning     getRcTuning();
    bool            setRcTuning(const MspRcTuning& t);
    MspPidAdvanced  getPidAdvanced();
    bool            setPidAdvanced(const MspPidAdvanced& p);
    MspFilterConfig getFilterConfig();
    bool            setFilterConfig(const MspFilterConfig& f);

    // ── Motors ───────────────────────────────────────────
    bool setMotors(const uint16_t motors[8]);

    // ── RC override ──────────────────────────────────────
    bool setRawRc(const uint16_t channels[], int count);

    // ── Config / system ──────────────────────────────────
    bool armingDisable(bool disable);
    bool reboot();
    bool saveEeprom();
    bool calibrateAccelerometer();
    bool calibrateMagnetometer();

    // ── OSD ──────────────────────────────────────────────
    MspOsdConfig    getOsdConfig();
    bool            setOsdConfig(const MspOsdConfig& c);

    // ── GPS ──────────────────────────────────────────────
    MspRawGps    getGps();
    MspGpsConfig getGpsConfig();
    bool         setGpsConfig(const MspGpsConfig& c);

    // ── Blackbox ─────────────────────────────────────────
    MspBlackboxConfig getBlackboxConfig();
    bool              setBlackboxConfig(const MspBlackboxConfig& c);

    // ── VTX ──────────────────────────────────────────────
    MspVtxConfig getVtxConfig();
    bool         setVtxConfig(const MspVtxConfig& c);

    // ── LED strip ────────────────────────────────────────
    std::vector<MspLedConfig> getLedStripConfig();
    bool setLedStripConfig(const std::vector<MspLedConfig>& leds);

    // ── Features ─────────────────────────────────────────
    uint32_t getFeatures();
    bool     setFeatures(uint32_t mask);

    // ── Serial ports ─────────────────────────────────────
    std::vector<MspSerialPortConfig> getSerialConfig();
    bool setSerialConfig(const std::vector<MspSerialPortConfig>& ports);

    // ── Mode ranges ──────────────────────────────────────
    std::vector<MspModeRange> getModeRanges();
    bool setModeRange(int index, const MspModeRange& range);

    // ── Motor config ─────────────────────────────────────
    MspMotorConfig getMotorConfig();
    bool           setMotorConfig(const MspMotorConfig& c);

    // ── Polling ──────────────────────────────────────────
    using TelemetryCallback = std::function<void(const MspMessage&)>;
    void startPolling(std::vector<MspCommand> commands,
                      int intervalMs,
                      TelemetryCallback cb);
    void stopPolling();

private:
    MspClient& client_;

    std::thread              pollThread_;
    std::atomic<bool>        polling_{false};
    std::mutex               pollMutex_;

    // Payload decode helpers
    template<typename T>
    static T requirePayload(const MspMessage& msg, size_t minSize) {
        if (msg.payload.size() < minSize)
            throw MspException(MspErrorCode::PayloadTooShort,
                               "Response payload too short");
        (void)minSize;
        T val{};
        return val;
    }

    const uint8_t* data(const MspMessage& msg) {
        return msg.payload.data();
    }

    void checkSize(const MspMessage& msg, size_t minSize) {
        if (msg.payload.size() < minSize)
            throw MspException(MspErrorCode::PayloadTooShort,
                               "Response payload too short");
    }
};

} // namespace msp
