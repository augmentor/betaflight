#include <msp/msp.h>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) { std::cerr << "Usage: pid_tuner <port>\n"; return 1; }

    try {
        msp::SerialPort port;
        port.open(argv[1], 115200);
        msp::MspClient   client(port);
        msp::MspCommands fc(client);

        // Read
        auto pid = fc.getPid();
        std::cout << "Current PID:\n"
                  << "  Roll:  P=" << (int)pid.roll.P
                  << " I=" << (int)pid.roll.I
                  << " D=" << (int)pid.roll.D << "\n"
                  << "  Pitch: P=" << (int)pid.pitch.P
                  << " I=" << (int)pid.pitch.I
                  << " D=" << (int)pid.pitch.D << "\n"
                  << "  Yaw:   P=" << (int)pid.yaw.P
                  << " I=" << (int)pid.yaw.I
                  << " D=" << (int)pid.yaw.D << "\n";

        // Example: bump roll P by 5
        pid.roll.P += 5;
        if (fc.setPid(pid)) {
            fc.saveEeprom();
            std::cout << "PID updated and saved.\n";
        } else {
            std::cerr << "Failed to set PID.\n";
        }

        port.close();
    } catch (const msp::MspException& e) {
        std::cerr << "MSP error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
