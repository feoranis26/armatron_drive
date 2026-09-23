#pragma once
#include <cmath>
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

namespace armatron {
enum class CommandKind { motion, stop, reset, connect };
struct Command {
    CommandKind kind;
    double x = 0, y = 0, yaw = 0;
};

inline bool decode_control_packet(const char* data, std::size_t size, Command& command) {
    if (size < 2 || static_cast<unsigned char>(data[0]) != 0xFA ||
        static_cast<unsigned char>(data[size-1]) != 0xFB)
        return false;
    std::istringstream input(std::string(data+1, size-2));
    std::vector<std::string> args;
    std::string token;
    while (input >> token) args.push_back(token);
    if (args.size() == 1) {
        if (args[0] == "safety_stop") command.kind = CommandKind::stop;
        else if (args[0] == "safety_reset") command.kind = CommandKind::reset;
        else if (args[0] == "connect") command.kind = CommandKind::connect;
        else return false;
        return true;
    }
    if (args.size() != 4 || args[0] != "whl") return false;
    try {
        auto number = [](const std::string& value) {
            std::size_t used = 0;
            double result = std::stod(value, &used);
            if (used != value.size() || !std::isfinite(result))
                throw std::invalid_argument("Invalid velocity");
            return result;
        };
        command.x = number(args[1]); command.y = number(args[2]); command.yaw = number(args[3]);
        command.kind = CommandKind::motion;
        return true;
    } catch (const std::exception&) { return false; }
}
}
