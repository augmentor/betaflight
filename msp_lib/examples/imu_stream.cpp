#include <msp/msp.h>
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    if (argc < 2) { std::cerr << "Usage: imu_stream <port>\n"; return 1; }

    try {
        msp::SerialPort port;
        port.open(argv[1], 115200);
        msp::MspClient   client(port);
        msp::MspCommands fc(client);

        fc.startPolling(
            { msp::MspCommand::MSP_RAW_IMU, msp::MspCommand::MSP_ATTITUDE },
            50,  // 20 Hz
            [](const msp::MspMessage& msg) {
                using namespace msp::codec;
                if (msg.payload.size() < 6) return;
                const uint8_t* p = msg.payload.data();
                if (msg.command ==
                    static_cast<uint16_t>(msp::MspCommand::MSP_ATTITUDE)) {
                    std::cout << "\rRoll=" << readS16(p)/10.0
                              << " Pitch=" << readS16(p+2)/10.0
                              << " Yaw=" << readS16(p+4)
                              << "    " << std::flush;
                }
            }
        );

        std::cout << "Streaming for 10 seconds...\n";
        std::this_thread::sleep_for(std::chrono::seconds(10));
        fc.stopPolling();
        std::cout << "\nDone.\n";

        port.close();
    } catch (const msp::MspException& e) {
        std::cerr << "MSP error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
