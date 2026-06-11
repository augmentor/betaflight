#include <msp/msp.h>
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    if (argc < 2) { std::cerr << "Usage: motor_test <port>\n"; return 1; }

    try {
        msp::SerialPort port;
        port.open(argv[1], 115200);
        msp::MspClient   client(port);
        msp::MspCommands fc(client);

        // Show current motor values
        auto motors = fc.getMotors();
        std::cout << "Motor values:";
        for (int i = 0; i < 8; ++i)
            std::cout << " M" << (i+1) << "=" << motors.motor[i];
        std::cout << "\n";

        // Spin motor 1 at minimum for 1 second (FC must NOT be armed!)
        std::cout << "Sending min throttle to all motors for 1s...\n";
        uint16_t vals[8] = {1000,1000,1000,1000,1000,1000,1000,1000};
        for (int i = 0; i < 20; ++i) {
            fc.setMotors(vals);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        port.close();
    } catch (const msp::MspException& e) {
        std::cerr << "MSP error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
