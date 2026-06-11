#include "msp/msp_commands.h"
#include <cstring>
#include <thread>
#include <chrono>

namespace msp {

using namespace codec;

MspCommands::MspCommands(MspClient& client) : client_(client) {}

MspCommands::~MspCommands() { stopPolling(); }

// ── Identification ────────────────────────────────────────

MspApiVersion MspCommands::getApiVersion() {
    auto msg = client_.request(MspCommand::MSP_API_VERSION);
    checkSize(msg, 3);
    const uint8_t* p = data(msg);
    return MspApiVersion{ p[0], p[1], p[2] };
}

std::string MspCommands::getFcVariant() {
    auto msg = client_.request(MspCommand::MSP_FC_VARIANT);
    checkSize(msg, 4);
    return std::string(reinterpret_cast<const char*>(data(msg)), 4);
}

std::string MspCommands::getFcVersion() {
    auto msg = client_.request(MspCommand::MSP_FC_VERSION);
    checkSize(msg, 3);
    const uint8_t* p = data(msg);
    return std::to_string(p[0]) + "." +
           std::to_string(p[1]) + "." +
           std::to_string(p[2]);
}

MspBoardInfo MspCommands::getBoardInfo() {
    auto msg = client_.request(MspCommand::MSP_BOARD_INFO);
    checkSize(msg, 6);
    const uint8_t* p = data(msg);
    size_t          sz = msg.payload.size();
    MspBoardInfo bi;
    bi.boardIdentifier  = std::string(reinterpret_cast<const char*>(p), 4);
    bi.hardwareRevision = readU16(p + 4);
    if (sz > 6) bi.boardType = p[6];
    if (sz > 7) bi.targetCapabilities = p[7];
    size_t off = 8;
    auto readStr = [&](std::string& s) {
        if (off >= sz) return;
        uint8_t len = p[off++];
        if (off + len > sz) return;
        s.assign(reinterpret_cast<const char*>(p + off), len);
        off += len;
    };
    readStr(bi.targetName);
    readStr(bi.boardName);
    readStr(bi.manufacturerId);
    if (off + 32 <= sz) {
        std::memcpy(bi.signature, p + off, 32);
        off += 32;
    }
    if (off < sz) bi.mcuTypeId = p[off];
    return bi;
}

MspBuildInfo MspCommands::getBuildInfo() {
    auto msg = client_.request(MspCommand::MSP_BUILD_INFO);
    checkSize(msg, 19);
    const uint8_t* p = data(msg);
    MspBuildInfo bi;
    bi.buildDate.assign(reinterpret_cast<const char*>(p),     11);
    bi.buildTime.assign(reinterpret_cast<const char*>(p + 11), 8);
    bi.shortGitRevision.assign(reinterpret_cast<const char*>(p + 19), 7);
    return bi;
}

std::string MspCommands::getName() {
    auto msg = client_.request(MspCommand::MSP_NAME);
    return std::string(msg.payload.begin(), msg.payload.end());
}

bool MspCommands::setName(const std::string& name) {
    std::vector<uint8_t> pl(name.begin(), name.end());
    return client_.send(MspCommand::MSP_SET_NAME, pl);
}

// ── Telemetry ─────────────────────────────────────────────

MspAttitude MspCommands::getAttitude() {
    auto msg = client_.request(MspCommand::MSP_ATTITUDE);
    checkSize(msg, 6);
    const uint8_t* p = data(msg);
    return MspAttitude{ readS16(p), readS16(p+2), readS16(p+4) };
}

MspRawImu MspCommands::getRawImu() {
    auto msg = client_.request(MspCommand::MSP_RAW_IMU);
    checkSize(msg, 18);
    const uint8_t* p = data(msg);
    MspRawImu imu;
    for (int i = 0; i < 3; ++i) imu.acc[i]  = readS16(p + i*2);
    for (int i = 0; i < 3; ++i) imu.gyro[i] = readS16(p + 6 + i*2);
    for (int i = 0; i < 3; ++i) imu.mag[i]  = readS16(p + 12 + i*2);
    return imu;
}

MspStatus MspCommands::getStatus() {
    auto msg = client_.request(MspCommand::MSP_STATUS);
    checkSize(msg, 11);
    const uint8_t* p = data(msg);
    MspStatus s;
    s.cycleTime    = readU16(p);
    s.i2cErrors    = readU16(p+2);
    s.sensorFlags  = readU16(p+4);
    s.flightModes  = readU32(p+6);
    s.configProfile= p[10];
    return s;
}

MspStatusEx MspCommands::getStatusEx() {
    auto msg = client_.request(MspCommand::MSP_STATUS_EX);
    checkSize(msg, 13);
    const uint8_t* p = data(msg);
    MspStatusEx s;
    s.cycleTime    = readU16(p);
    s.i2cErrors    = readU16(p+2);
    s.sensorFlags  = readU16(p+4);
    s.flightModes  = readU32(p+6);
    s.configProfile= p[10];
    s.averageSystemLoadPercent = readU16(p+11);
    if (msg.payload.size() >= 15) {
        s.armingFlags           = p[13];
        s.accCalibrationAxisFlags = p[14];
    }
    return s;
}

MspMotors MspCommands::getMotors() {
    auto msg = client_.request(MspCommand::MSP_MOTOR);
    checkSize(msg, 16);
    const uint8_t* p = data(msg);
    MspMotors m;
    for (int i = 0; i < 8; ++i) m.motor[i] = readU16(p + i*2);
    return m;
}

MspRcChannels MspCommands::getRc() {
    auto msg = client_.request(MspCommand::MSP_RC);
    MspRcChannels rc;
    rc.count = static_cast<int>(msg.payload.size() / 2);
    if (rc.count > 18) rc.count = 18;
    const uint8_t* p = data(msg);
    for (int i = 0; i < rc.count; ++i) rc.channel[i] = readU16(p + i*2);
    return rc;
}

MspAnalog MspCommands::getAnalog() {
    auto msg = client_.request(MspCommand::MSP_ANALOG);
    checkSize(msg, 7);
    const uint8_t* p = data(msg);
    MspAnalog a;
    a.vbat      = p[0];
    a.mAhDrawn  = readU16(p+1);
    a.rssi      = readU16(p+3);
    a.amperage  = readS16(p+5);
    if (msg.payload.size() >= 9) a.voltage = readU16(p+7);
    return a;
}

MspBatteryState MspCommands::getBatteryState() {
    auto msg = client_.request(MspCommand::MSP_BATTERY_STATE);
    checkSize(msg, 9);
    const uint8_t* p = data(msg);
    MspBatteryState b;
    b.cellCount = p[0];
    b.capacity  = readU16(p+1);
    b.voltage   = p[3];
    b.mAhDrawn  = readU16(p+4);
    b.amperage  = readS16(p+6);
    b.state     = p[8];
    return b;
}

int MspCommands::getAltitude() {
    auto msg = client_.request(MspCommand::MSP_ALTITUDE);
    checkSize(msg, 4);
    return static_cast<int>(readS32(data(msg)));
}

// ── PID ───────────────────────────────────────────────────

MspPid MspCommands::getPid() {
    auto msg = client_.request(MspCommand::MSP_PID);
    checkSize(msg, 9);
    const uint8_t* p = data(msg);
    MspPid pid;
    pid.roll  = { p[0], p[1], p[2] };
    pid.pitch = { p[3], p[4], p[5] };
    pid.yaw   = { p[6], p[7], p[8] };
    return pid;
}

bool MspCommands::setPid(const MspPid& pid) {
    std::vector<uint8_t> pl;
    writeU8(pl, pid.roll.P);  writeU8(pl, pid.roll.I);  writeU8(pl, pid.roll.D);
    writeU8(pl, pid.pitch.P); writeU8(pl, pid.pitch.I); writeU8(pl, pid.pitch.D);
    writeU8(pl, pid.yaw.P);   writeU8(pl, pid.yaw.I);   writeU8(pl, pid.yaw.D);
    return client_.send(MspCommand::MSP_SET_PID, pl);
}

MspRcTuning MspCommands::getRcTuning() {
    auto msg = client_.request(MspCommand::MSP_RC_TUNING);
    checkSize(msg, 10);
    const uint8_t* p = data(msg);
    MspRcTuning t;
    t.rcRate         = p[0];
    t.rcExpo         = p[1];
    t.rollPitchRate  = p[2];
    t.yawRate        = p[3];
    t.yawExpo        = p[4];
    t.dynThrPID      = p[5];
    t.thrMid         = p[6];
    t.thrExpo        = p[7];
    t.tpaBreakpoint  = readU16(p+8);
    if (msg.payload.size() >= 11) t.rcYawExpo = p[10];
    if (msg.payload.size() >= 12) t.rcYawRate = p[11];
    if (msg.payload.size() >= 13) t.rollRate  = p[12];
    if (msg.payload.size() >= 14) t.pitchRate = p[13];
    return t;
}

bool MspCommands::setRcTuning(const MspRcTuning& t) {
    std::vector<uint8_t> pl;
    writeU8(pl, t.rcRate);   writeU8(pl, t.rcExpo);
    writeU8(pl, t.rollPitchRate);
    writeU8(pl, t.yawRate);  writeU8(pl, t.yawExpo);
    writeU8(pl, t.dynThrPID);
    writeU8(pl, t.thrMid);   writeU8(pl, t.thrExpo);
    writeU16(pl, t.tpaBreakpoint);
    writeU8(pl, t.rcYawExpo); writeU8(pl, t.rcYawRate);
    writeU8(pl, t.rollRate);  writeU8(pl, t.pitchRate);
    return client_.send(MspCommand::MSP_SET_RC_TUNING, pl);
}

MspPidAdvanced MspCommands::getPidAdvanced() {
    auto msg = client_.request(MspCommand::MSP_PID_ADVANCED);
    const uint8_t* p = data(msg);
    MspPidAdvanced pa{};
    size_t sz = msg.payload.size();
    if (sz < 17) return pa;
    pa.rollPitchItermIgnoreRate = readU16(p);
    pa.yawItermIgnoreRate       = readU16(p+2);
    pa.yawPLimit                = readU16(p+4);
    pa.deltaMethod              = p[6];
    pa.vbatPidCompensation      = p[7];
    pa.feedforwardTransition    = p[8];
    pa.dtermSetpointWeight      = p[9];
    pa.toleranceBand            = p[10];
    pa.toleranceBandReduction   = p[11];
    pa.itermThrottleGain        = p[12];
    pa.pidMaxVelocity           = readU16(p+13);
    pa.pidMaxVelocityYaw        = readU16(p+15);
    if (sz >= 19) pa.levelAngleLimit  = p[17];
    if (sz >= 20) pa.levelSensitivity = p[18];
    return pa;
}

bool MspCommands::setPidAdvanced(const MspPidAdvanced& pa) {
    std::vector<uint8_t> pl;
    writeU16(pl, pa.rollPitchItermIgnoreRate);
    writeU16(pl, pa.yawItermIgnoreRate);
    writeU16(pl, pa.yawPLimit);
    writeU8(pl,  pa.deltaMethod);
    writeU8(pl,  pa.vbatPidCompensation);
    writeU8(pl,  pa.feedforwardTransition);
    writeU8(pl,  pa.dtermSetpointWeight);
    writeU8(pl,  pa.toleranceBand);
    writeU8(pl,  pa.toleranceBandReduction);
    writeU8(pl,  pa.itermThrottleGain);
    writeU16(pl, pa.pidMaxVelocity);
    writeU16(pl, pa.pidMaxVelocityYaw);
    writeU8(pl,  pa.levelAngleLimit);
    writeU8(pl,  pa.levelSensitivity);
    return client_.send(MspCommand::MSP_SET_PID_ADVANCED, pl);
}

MspFilterConfig MspCommands::getFilterConfig() {
    auto msg = client_.request(MspCommand::MSP_FILTER_CONFIG);
    checkSize(msg, 13);
    const uint8_t* p = data(msg);
    MspFilterConfig f{};
    f.gyroSoftLpfHz         = p[0];
    f.dtermLpfHz            = readU16(p+1);
    f.yawLpfHz              = readU16(p+3);
    f.gyroSoftNotchHz1      = readU16(p+5);
    f.gyroSoftNotchCutoff1  = readU16(p+7);
    f.dtermNotchHz          = readU16(p+9);
    f.dtermNotchCutoff      = readU16(p+11);
    if (msg.payload.size() >= 15) f.gyroSoftNotchHz2    = readU16(p+13);
    if (msg.payload.size() >= 17) f.gyroSoftNotchCutoff2= readU16(p+15);
    return f;
}

bool MspCommands::setFilterConfig(const MspFilterConfig& f) {
    std::vector<uint8_t> pl;
    writeU8(pl,  f.gyroSoftLpfHz);
    writeU16(pl, f.dtermLpfHz);
    writeU16(pl, f.yawLpfHz);
    writeU16(pl, f.gyroSoftNotchHz1);
    writeU16(pl, f.gyroSoftNotchCutoff1);
    writeU16(pl, f.dtermNotchHz);
    writeU16(pl, f.dtermNotchCutoff);
    writeU16(pl, f.gyroSoftNotchHz2);
    writeU16(pl, f.gyroSoftNotchCutoff2);
    return client_.send(MspCommand::MSP_SET_FILTER_CONFIG, pl);
}

// ── Motors ────────────────────────────────────────────────

bool MspCommands::setMotors(const uint16_t motors[8]) {
    std::vector<uint8_t> pl;
    for (int i = 0; i < 8; ++i) writeU16(pl, motors[i]);
    return client_.send(MspCommand::MSP_SET_MOTOR, pl);
}

bool MspCommands::setRawRc(const uint16_t channels[], int count) {
    if (count > 18) count = 18;
    std::vector<uint8_t> pl;
    for (int i = 0; i < count; ++i) writeU16(pl, channels[i]);
    return client_.send(MspCommand::MSP_SET_RAW_RC, pl);
}

// ── Config / system ───────────────────────────────────────

bool MspCommands::armingDisable(bool disable) {
    std::vector<uint8_t> pl;
    writeU8(pl, disable ? 1u : 0u);
    writeU32(pl, 0); // runaway takeoff prevention bits
    return client_.send(MspCommand::MSP_ARMING_DISABLE, pl);
}

bool MspCommands::reboot() {
    return client_.send(MspCommand::MSP_REBOOT);
}

bool MspCommands::saveEeprom() {
    return client_.send(MspCommand::MSP_EEPROM_WRITE);
}

bool MspCommands::calibrateAccelerometer() {
    return client_.send(MspCommand::MSP_ACC_CALIBRATION);
}

bool MspCommands::calibrateMagnetometer() {
    return client_.send(MspCommand::MSP_MAG_CALIBRATION);
}

// ── OSD ───────────────────────────────────────────────────

MspOsdConfig MspCommands::getOsdConfig() {
    auto msg = client_.request(MspCommand::MSP_OSD_CONFIG);
    checkSize(msg, 10);
    const uint8_t* p = data(msg);
    MspOsdConfig c;
    c.videoSystem = p[0];
    c.units       = p[1];
    c.rssiAlarm   = p[2];
    c.capAlarm    = readU16(p+3);
    c.timeAlarm   = readU16(p+5);
    c.altAlarm    = readU16(p+7);
    return c;
}

bool MspCommands::setOsdConfig(const MspOsdConfig& c) {
    std::vector<uint8_t> pl;
    writeU8(pl,  c.videoSystem);
    writeU8(pl,  c.units);
    writeU8(pl,  c.rssiAlarm);
    writeU16(pl, c.capAlarm);
    writeU16(pl, c.timeAlarm);
    writeU16(pl, c.altAlarm);
    return client_.send(MspCommand::MSP_SET_OSD_CONFIG, pl);
}

// ── GPS ───────────────────────────────────────────────────

MspRawGps MspCommands::getGps() {
    auto msg = client_.request(MspCommand::MSP_RAW_GPS);
    checkSize(msg, 16);
    const uint8_t* p = data(msg);
    MspRawGps g;
    g.fixType     = p[0];
    g.numSat      = p[1];
    g.lat         = readS32(p+2);
    g.lon         = readS32(p+6);
    g.altCm       = readS16(p+10);
    g.groundSpeed = readU16(p+12);
    g.groundCourse= readU16(p+14);
    if (msg.payload.size() >= 18) g.hdop = readU16(p+16);
    return g;
}

MspGpsConfig MspCommands::getGpsConfig() {
    auto msg = client_.request(MspCommand::MSP_GPS_CONFIG);
    checkSize(msg, 4);
    const uint8_t* p = data(msg);
    MspGpsConfig c{};
    c.provider   = p[0];
    c.sbasMode   = p[1];
    c.autoConfig = p[2];
    c.autoBaud   = p[3];
    return c;
}

bool MspCommands::setGpsConfig(const MspGpsConfig& c) {
    std::vector<uint8_t> pl;
    writeU8(pl, c.provider);
    writeU8(pl, c.sbasMode);
    writeU8(pl, c.autoConfig);
    writeU8(pl, c.autoBaud);
    return client_.send(MspCommand::MSP_SET_GPS_CONFIG, pl);
}

// ── Blackbox ──────────────────────────────────────────────

MspBlackboxConfig MspCommands::getBlackboxConfig() {
    auto msg = client_.request(MspCommand::MSP_BLACKBOX_CONFIG);
    checkSize(msg, 4);
    const uint8_t* p = data(msg);
    return MspBlackboxConfig{ p[0], p[1], p[2], readU16(p+3) };
}

bool MspCommands::setBlackboxConfig(const MspBlackboxConfig& c) {
    std::vector<uint8_t> pl;
    writeU8(pl,  c.device);
    writeU8(pl,  c.rateNum);
    writeU8(pl,  c.rateDenom);
    writeU16(pl, c.pDenom);
    return client_.send(MspCommand::MSP_SET_BLACKBOX_CONFIG, pl);
}

// ── VTX ───────────────────────────────────────────────────

MspVtxConfig MspCommands::getVtxConfig() {
    auto msg = client_.request(MspCommand::MSP_VTX_CONFIG);
    checkSize(msg, 10);
    const uint8_t* p = data(msg);
    MspVtxConfig c{};
    c.vtxType   = p[0];
    c.band      = p[1];
    c.channel   = p[2];
    c.power     = p[3];
    c.pitMode   = p[4];
    c.freq      = readU16(p+5);
    if (msg.payload.size() >= 8)  c.deviceIsReady  = p[7];
    if (msg.payload.size() >= 9)  c.lowPowerDisarm = p[8];
    if (msg.payload.size() >= 11) c.pitModeFreq    = readU16(p+9);
    return c;
}

bool MspCommands::setVtxConfig(const MspVtxConfig& c) {
    std::vector<uint8_t> pl;
    writeU16(pl, c.freq);
    writeU8(pl,  c.band);
    writeU8(pl,  c.channel);
    writeU8(pl,  c.power);
    writeU8(pl,  c.pitMode);
    return client_.send(MspCommand::MSP_SET_VTX_CONFIG, pl);
}

// ── LED strip ─────────────────────────────────────────────

std::vector<MspLedConfig> MspCommands::getLedStripConfig() {
    auto msg = client_.request(MspCommand::MSP_LED_STRIP_CONFIG);
    std::vector<MspLedConfig> leds;
    const uint8_t* p = data(msg);
    size_t count = msg.payload.size() / 5;
    for (size_t i = 0; i < count; ++i) {
        MspLedConfig lc;
        lc.config = readU32(p + i*5);
        lc.color  = p[i*5 + 4];
        leds.push_back(lc);
    }
    return leds;
}

bool MspCommands::setLedStripConfig(const std::vector<MspLedConfig>& leds) {
    for (size_t i = 0; i < leds.size(); ++i) {
        std::vector<uint8_t> pl;
        writeU8(pl, static_cast<uint8_t>(i));
        writeU32(pl, leds[i].config);
        writeU8(pl, leds[i].color);
        if (!client_.send(MspCommand::MSP_SET_LED_STRIP_CONFIG, pl))
            return false;
    }
    return true;
}

// ── Features ──────────────────────────────────────────────

uint32_t MspCommands::getFeatures() {
    auto msg = client_.request(MspCommand::MSP_FEATURE_CONFIG);
    checkSize(msg, 4);
    return readU32(data(msg));
}

bool MspCommands::setFeatures(uint32_t mask) {
    std::vector<uint8_t> pl;
    writeU32(pl, mask);
    return client_.send(MspCommand::MSP_SET_FEATURE_CONFIG, pl);
}

// ── Serial ports ──────────────────────────────────────────

std::vector<MspSerialPortConfig> MspCommands::getSerialConfig() {
    auto msg = client_.request(MspCommand::MSP_CF_SERIAL_CONFIG);
    std::vector<MspSerialPortConfig> ports;
    const uint8_t* p = data(msg);
    size_t count = msg.payload.size() / 7;
    for (size_t i = 0; i < count; ++i) {
        MspSerialPortConfig pc;
        pc.identifier             = p[i*7 + 0];
        pc.functionMask           = readU16(p + i*7 + 1);
        pc.mspBaudrateIndex       = p[i*7 + 3];
        pc.gpsBaudrateIndex       = p[i*7 + 4];
        pc.telemetryBaudrateIndex = p[i*7 + 5];
        pc.peripheralBaudrateIndex= p[i*7 + 6];
        ports.push_back(pc);
    }
    return ports;
}

bool MspCommands::setSerialConfig(
        const std::vector<MspSerialPortConfig>& ports) {
    std::vector<uint8_t> pl;
    for (const auto& pc : ports) {
        writeU8(pl,  pc.identifier);
        writeU16(pl, pc.functionMask);
        writeU8(pl,  pc.mspBaudrateIndex);
        writeU8(pl,  pc.gpsBaudrateIndex);
        writeU8(pl,  pc.telemetryBaudrateIndex);
        writeU8(pl,  pc.peripheralBaudrateIndex);
    }
    return client_.send(MspCommand::MSP_SET_CF_SERIAL_CONFIG, pl);
}

// ── Mode ranges ───────────────────────────────────────────

std::vector<MspModeRange> MspCommands::getModeRanges() {
    auto msg = client_.request(MspCommand::MSP_MODE_RANGES);
    std::vector<MspModeRange> ranges;
    const uint8_t* p = data(msg);
    size_t count = msg.payload.size() / 4;
    for (size_t i = 0; i < count; ++i) {
        MspModeRange r;
        r.boxId            = p[i*4 + 0];
        r.auxChannelIndex  = p[i*4 + 1];
        r.rangeStartStep   = p[i*4 + 2];
        r.rangeEndStep     = p[i*4 + 3];
        ranges.push_back(r);
    }
    return ranges;
}

bool MspCommands::setModeRange(int index, const MspModeRange& range) {
    std::vector<uint8_t> pl;
    writeU8(pl, static_cast<uint8_t>(index));
    writeU8(pl, range.boxId);
    writeU8(pl, range.auxChannelIndex);
    writeU8(pl, range.rangeStartStep);
    writeU8(pl, range.rangeEndStep);
    return client_.send(MspCommand::MSP_SET_MODE_RANGE, pl);
}

// ── Motor config ──────────────────────────────────────────

MspMotorConfig MspCommands::getMotorConfig() {
    auto msg = client_.request(MspCommand::MSP_MOTOR_CONFIG);
    checkSize(msg, 6);
    const uint8_t* p = data(msg);
    MspMotorConfig c{};
    c.minThrottle = readU16(p);
    c.maxThrottle = readU16(p+2);
    c.minCommand  = readU16(p+4);
    if (msg.payload.size() >= 8) {
        c.motorCount       = p[6];
        c.motorPoles       = p[7];
    }
    if (msg.payload.size() >= 10) {
        c.useDshotTelemetry = p[8];
        c.useEscSensor      = p[9];
    }
    return c;
}

bool MspCommands::setMotorConfig(const MspMotorConfig& c) {
    std::vector<uint8_t> pl;
    writeU16(pl, c.minThrottle);
    writeU16(pl, c.maxThrottle);
    writeU16(pl, c.minCommand);
    return client_.send(MspCommand::MSP_SET_MOTOR_CONFIG, pl);
}

// ── Polling ───────────────────────────────────────────────

void MspCommands::startPolling(std::vector<MspCommand> commands,
                                int intervalMs,
                                TelemetryCallback cb) {
    stopPolling();
    polling_ = true;
    pollThread_ = std::thread([this, commands, intervalMs, cb]() {
        while (polling_) {
            auto t0 = std::chrono::steady_clock::now();
            for (const auto& cmd : commands) {
                if (!polling_) break;
                try {
                    std::lock_guard<std::mutex> lock(pollMutex_);
                    auto msg = client_.request(cmd);
                    cb(msg);
                } catch (...) {}
            }
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - t0).count();
            int remaining = intervalMs - static_cast<int>(elapsed);
            if (remaining > 0)
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(remaining));
        }
    });
}

void MspCommands::stopPolling() {
    polling_ = false;
    if (pollThread_.joinable()) pollThread_.join();
}

} // namespace msp
