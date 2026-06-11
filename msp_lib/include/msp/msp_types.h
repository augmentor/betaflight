#pragma once
#include <cstdint>
#include <string>

namespace msp {

enum class MspVersion { V1, V2 };

enum class MspDirection : uint8_t {
    Request  = '<',
    Response = '>',
    Error    = '!'
};

enum class MspCommand : uint16_t {
    // Identification
    MSP_API_VERSION          = 1,
    MSP_FC_VARIANT           = 2,
    MSP_FC_VERSION           = 3,
    MSP_BOARD_INFO           = 4,
    MSP_BUILD_INFO           = 5,
    MSP_NAME                 = 10,
    MSP_SET_NAME             = 11,

    // Mode ranges
    MSP_MODE_RANGES          = 34,
    MSP_SET_MODE_RANGE       = 35,
    MSP_FEATURE_CONFIG       = 36,
    MSP_SET_FEATURE_CONFIG   = 37,
    MSP_BOARD_ALIGNMENT_CONFIG = 38,
    MSP_SET_BOARD_ALIGNMENT_CONFIG = 39,

    // Adjustment
    MSP_ADJUSTMENT_RANGES    = 52,
    MSP_SET_ADJUSTMENT_RANGE = 53,
    MSP_CF_SERIAL_CONFIG     = 54,
    MSP_SET_CF_SERIAL_CONFIG = 55,

    // RX
    MSP_RX_CONFIG            = 44,
    MSP_SET_RX_CONFIG        = 45,
    MSP_LED_COLORS           = 46,
    MSP_SET_LED_COLORS       = 50,
    MSP_LED_STRIP_CONFIG     = 48,
    MSP_SET_LED_STRIP_CONFIG = 49,
    MSP_RX_MAP               = 64,
    MSP_SET_RX_MAP           = 65,

    // Dataflash
    MSP_DATAFLASH_SUMMARY    = 70,
    MSP_DATAFLASH_READ       = 71,
    MSP_DATAFLASH_ERASE      = 72,

    // RX fail
    MSP_RXFAIL_CONFIG        = 77,
    MSP_SET_RXFAIL_CONFIG    = 78,
    MSP_SDCARD_SUMMARY       = 79,

    // Blackbox
    MSP_BLACKBOX_CONFIG      = 80,
    MSP_SET_BLACKBOX_CONFIG  = 81,

    // OSD
    MSP_OSD_CONFIG           = 84,
    MSP_SET_OSD_CONFIG       = 85,
    MSP_OSD_CHAR_READ        = 86,
    MSP_OSD_CHAR_WRITE       = 87,

    // VTX
    MSP_VTX_CONFIG           = 88,
    MSP_SET_VTX_CONFIG       = 89,

    // Filter/PID advanced
    MSP_FILTER_CONFIG        = 92,
    MSP_SET_FILTER_CONFIG    = 93,
    MSP_PID_ADVANCED         = 94,
    MSP_SET_PID_ADVANCED     = 95,
    MSP_SENSOR_CONFIG        = 96,
    MSP_SET_SENSOR_CONFIG    = 97,

    // Arming
    MSP_ARMING_CONFIG        = 61,
    MSP_SET_ARMING_CONFIG    = 62,
    MSP_ARMING_DISABLE       = 99,

    // Core telemetry
    MSP_STATUS               = 101,
    MSP_RAW_IMU              = 102,
    MSP_SERVO                = 103,
    MSP_MOTOR                = 104,
    MSP_RC                   = 105,
    MSP_RAW_GPS              = 106,
    MSP_COMP_GPS             = 107,
    MSP_ATTITUDE             = 108,
    MSP_ALTITUDE             = 109,
    MSP_ANALOG               = 110,
    MSP_RC_TUNING            = 111,
    MSP_PID                  = 112,
    MSP_ACTIVEBOXES          = 113,
    MSP_MISC                 = 114,
    MSP_MOTOR_PINS           = 115,
    MSP_BOXNAMES             = 116,
    MSP_PIDNAMES             = 117,
    MSP_WP                   = 118,
    MSP_BOXIDS               = 119,
    MSP_SERVO_CONFIGURATIONS = 120,

    // Battery/power
    MSP_VOLTAGE_METERS       = 128,
    MSP_CURRENT_METERS       = 129,
    MSP_BATTERY_STATE        = 130,
    MSP_MOTOR_CONFIG         = 131,
    MSP_SET_MOTOR_CONFIG     = 131,
    MSP_GPS_CONFIG           = 132,
    MSP_SET_GPS_CONFIG       = 133,
    MSP_GPS_RESCUE           = 135,
    MSP_SET_GPS_RESCUE       = 136,

    // LED strip modecolor
    MSP_LED_STRIP_MODECOLOR  = 127,
    MSP_SET_LED_STRIP_MODECOLOR = 221,

    // Mixer
    MSP_MIXER_CONFIG         = 42,
    MSP_SET_MIXER_CONFIG     = 43,
    MSP_MOTOR_3D_CONFIG      = 124,
    MSP_SENSOR_ALIGNMENT     = 126,

    MSP_STATUS_EX            = 150,

    // SET commands
    MSP_SET_RAW_RC           = 200,
    MSP_SET_RAW_GPS          = 201,
    MSP_SET_PID              = 202,
    MSP_SET_RC_TUNING        = 204,
    MSP_ACC_CALIBRATION      = 205,
    MSP_MAG_CALIBRATION      = 206,
    MSP_RESET_CONF           = 208,
    MSP_SET_WP               = 209,
    MSP_SELECT_SETTING       = 210,
    MSP_SET_HEAD             = 211,
    MSP_SET_SERVO_CONFIGURATION = 212,
    MSP_SET_MOTOR            = 214,
    MSP_SET_BEEPER_CONFIG    = 39,
    MSP_BEEPER_CONFIG        = 37,

    // System
    MSP_REBOOT               = 68,
    MSP_EEPROM_WRITE         = 250,

    // MSP v2 only
    MSP2_COMMON_SETTING_INFO    = 0x1009,
    MSP2_COMMON_SERIAL_CONFIG   = 0x100A,
    MSP2_COMMON_MOTOR_MIXER     = 0x1005,
    MSP2_COMMON_SET_MOTOR_MIXER = 0x1006,
    MSP2_INAV_MIXER             = 0x2010,
    MSP2_SENSOR_GPS             = 0x1F03,
    MSP2_SENSOR_RANGEFINDER     = 0x1F01,
    MSP2_SENSOR_OPTIC_FLOW      = 0x1F02,
    MSP2_SENSOR_BARO            = 0x1F04,
    MSP2_GET_TEXT               = 0x3006,
    MSP2_SET_TEXT               = 0x3007,
    MSP2_MOTOR_OUTPUT_REORDERING = 0x3001,
    MSP2_SET_MOTOR_OUTPUT_REORDERING = 0x3002,
    MSP2_SEND_DSHOT_COMMAND     = 0x3003,
    MSP2_VTX_DEVICE_STATUS      = 0x3004,
    MSP2_OSD_WARNINGS           = 0x3005,
    MSP2_MCU_INFO               = 0x300C,
};

// ── Data structures ──────────────────────────────────────

struct MspAttitude {
    int16_t roll;   // 1/10 degree
    int16_t pitch;  // 1/10 degree
    int16_t yaw;    // degrees
};

struct MspStatus {
    uint16_t cycleTime;
    uint16_t i2cErrors;
    uint16_t sensorFlags;
    uint32_t flightModes;
    uint8_t  configProfile;
};

struct MspStatusEx {
    uint16_t cycleTime;
    uint16_t i2cErrors;
    uint16_t sensorFlags;
    uint32_t flightModes;
    uint8_t  configProfile;
    uint16_t averageSystemLoadPercent;
    uint8_t  armingFlags;
    uint8_t  accCalibrationAxisFlags;
};

struct MspRawImu {
    int16_t acc[3];
    int16_t gyro[3];
    int16_t mag[3];
};

struct MspPid {
    struct Axis { uint8_t P, I, D; };
    Axis roll, pitch, yaw;
};

struct MspMotors {
    uint16_t motor[8];
};

struct MspRcChannels {
    uint16_t channel[18];
    int count;
};

struct MspAnalog {
    uint8_t  vbat;       // 1/10 V
    uint16_t mAhDrawn;
    uint16_t rssi;       // 0-1023
    int16_t  amperage;   // 1/100 A
    uint16_t voltage;    // 1/100 V
};

struct MspBatteryState {
    uint8_t  cellCount;
    uint16_t capacity;
    uint8_t  voltage;    // 1/10 V
    uint16_t mAhDrawn;
    int16_t  amperage;
    uint8_t  state;      // 0=OK 1=WARN 2=CRIT 3=ABSENT
};

struct MspRcTuning {
    uint8_t  rcRate;
    uint8_t  rcExpo;
    uint8_t  rollPitchRate;
    uint8_t  yawRate;
    uint8_t  yawExpo;
    uint8_t  dynThrPID;
    uint8_t  thrMid;
    uint8_t  thrExpo;
    uint16_t tpaBreakpoint;
    uint8_t  rcYawExpo;
    uint8_t  rcYawRate;
    uint8_t  rollRate;
    uint8_t  pitchRate;
};

struct MspPidAdvanced {
    uint16_t rollPitchItermIgnoreRate;
    uint16_t yawItermIgnoreRate;
    uint16_t yawPLimit;
    uint8_t  deltaMethod;
    uint8_t  vbatPidCompensation;
    uint8_t  feedforwardTransition;
    uint8_t  dtermSetpointWeight;
    uint8_t  toleranceBand;
    uint8_t  toleranceBandReduction;
    uint8_t  itermThrottleGain;
    uint16_t pidMaxVelocity;
    uint16_t pidMaxVelocityYaw;
    uint8_t  levelAngleLimit;
    uint8_t  levelSensitivity;
    uint16_t itermThrottleThreshold;
    uint16_t itermAcceleratorGain;
    uint16_t dtermSetpointWeight2;
    uint8_t  itermRotation;
    uint8_t  smartFeedforward;
    uint8_t  itermRelax;
    uint8_t  itermRelaxType;
    uint8_t  absoluteControl;
    uint8_t  throttleBoost;
    uint8_t  acrobaticsHelper;
    uint8_t  feedforwardRoll;
    uint8_t  feedforwardPitch;
    uint8_t  feedforwardYaw;
    uint8_t  feedforwardLimitPercent;
    uint8_t  feedforwardSmoothFactor;
    uint8_t  dMinRoll;
    uint8_t  dMinPitch;
    uint8_t  dMinYaw;
    uint8_t  dMinGain;
    uint8_t  dMinAdvance;
    uint8_t  useIntegratedYaw;
    uint8_t  integratedYawRelax;
    uint8_t  itermRelaxCutoff;
    uint8_t  motorOutputLimit;
    int8_t   autoProfileCellCount;
    uint8_t  idleMinRpm;
    uint8_t  antiGravityMode;
};

struct MspFilterConfig {
    uint8_t  gyroSoftLpfHz;
    uint16_t dtermLpfHz;
    uint16_t yawLpfHz;
    uint16_t gyroSoftNotchHz1;
    uint16_t gyroSoftNotchCutoff1;
    uint16_t dtermNotchHz;
    uint16_t dtermNotchCutoff;
    uint16_t gyroSoftNotchHz2;
    uint16_t gyroSoftNotchCutoff2;
    uint8_t  dtermFilterType;
    uint8_t  gyroHardwareLpfHz;
    uint8_t  gyroLowpassType;
    uint16_t gyroLowpassHz;
    uint16_t gyroLowpass2Hz;
    uint8_t  gyroLowpass2Type;
    uint16_t dtermLowpass2Hz;
    uint8_t  dtermFilterType2;
    uint8_t  gyroFilterDebug;
};

struct MspRawGps {
    uint8_t  fixType;    // 0=none 1=2D 2=3D
    uint8_t  numSat;
    int32_t  lat;        // 1e-7 degrees
    int32_t  lon;        // 1e-7 degrees
    int16_t  altCm;      // meters
    uint16_t groundSpeed; // cm/s
    uint16_t groundCourse; // 1/10 degrees
    uint16_t hdop;
};

struct MspGpsConfig {
    uint8_t provider;
    uint8_t sbasMode;
    uint8_t autoConfig;
    uint8_t autoBaud;
    uint8_t homePointOnce;
    uint8_t ubloxUseGalileo;
};

struct MspOsdConfig {
    uint8_t  videoSystem;
    uint8_t  units;
    uint8_t  rssiAlarm;
    uint16_t capAlarm;
    uint16_t timeAlarm;
    uint16_t altAlarm;
};

struct MspBlackboxConfig {
    uint8_t  device;
    uint8_t  rateNum;
    uint8_t  rateDenom;
    uint16_t pDenom;
};

struct MspVtxConfig {
    uint8_t  vtxType;
    uint8_t  band;
    uint8_t  channel;
    uint8_t  power;
    uint8_t  pitMode;
    uint16_t freq;
    uint8_t  deviceIsReady;
    uint8_t  lowPowerDisarm;
    uint16_t pitModeFreq;
    uint8_t  vtxTableAvailable;
    uint8_t  bands;
    uint8_t  channels;
    uint8_t  powerLevels;
};

struct MspLedConfig {
    uint32_t config;
    uint8_t  color;
};

struct MspSerialPortConfig {
    uint8_t  identifier;
    uint16_t functionMask;
    uint8_t  mspBaudrateIndex;
    uint8_t  gpsBaudrateIndex;
    uint8_t  telemetryBaudrateIndex;
    uint8_t  peripheralBaudrateIndex;
};

struct MspModeRange {
    uint8_t boxId;
    uint8_t auxChannelIndex;
    uint8_t rangeStartStep;
    uint8_t rangeEndStep;
};

struct MspMotorConfig {
    uint16_t minThrottle;
    uint16_t maxThrottle;
    uint16_t minCommand;
    uint8_t  motorCount;
    uint8_t  motorPoles;
    uint8_t  useDshotTelemetry;
    uint8_t  useEscSensor;
};

struct MspBoardInfo {
    std::string boardIdentifier;  // 4 chars
    uint16_t    hardwareRevision;
    uint8_t     boardType;
    uint8_t     targetCapabilities;
    std::string targetName;
    std::string boardName;
    std::string manufacturerId;
    uint8_t     signature[32];
    uint8_t     mcuTypeId;
};

struct MspApiVersion {
    uint8_t protocolVersion;
    uint8_t apiVersionMajor;
    uint8_t apiVersionMinor;
};

struct MspBuildInfo {
    std::string buildDate;   // 11 chars
    std::string buildTime;   // 8 chars
    std::string shortGitRevision; // 7 chars
};

} // namespace msp
