#include <msp/msp.h>
#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[]) {
    std::string portName = (argc > 1) ? argv[1] : "";

    if (portName.empty()) {
        auto ports = msp::SerialPort::listPorts();
        if (ports.empty()) {
            std::cerr << "No serial ports found.\n";
            return 1;
        }
        std::cout << "Available ports:\n";
        for (const auto& p : ports)
            std::cout << "  " << p.port << "  " << p.description << "\n";
        portName = ports.front().port;
        std::cout << "Using: " << portName << "\n\n";
    }

    try {
        msp::SerialPort port;
        port.open(portName, 115200);

        msp::MspClient  client(port);
        msp::MspCommands fc(client);

        auto ver = client.detectVersion();
        std::cout << "Protocol: MSP "
                  << (ver == msp::MspVersion::V2 ? "v2" : "v1") << "\n";

        std::cout << "FC:  " << fc.getFcVariant()
                  << "  " << fc.getFcVersion() << "\n";

        auto att = fc.getAttitude();
        std::cout << "Roll:  " << att.roll  / 10.0 << " deg\n";
        std::cout << "Pitch: " << att.pitch / 10.0 << " deg\n";
        std::cout << "Yaw:   " << att.yaw           << " deg\n";

        auto st = fc.getStatus();
        std::cout << "Cycle time: " << st.cycleTime << " us\n";
        std::cout << "Sensors:    0x" << std::hex << st.sensorFlags
                  << std::dec << "\n";

        auto bat = fc.getAnalog();
        std::cout << "Vbat:  " << bat.vbat / 10.0    << " V\n";
        std::cout << "RSSI:  " << bat.rssi            << "\n";
        std::cout << "mAh:   " << bat.mAhDrawn        << "\n";

        port.close();
    } catch (const msp::MspException& e) {
        std::cerr << "MSP error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
