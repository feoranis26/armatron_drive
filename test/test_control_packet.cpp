#include "control_packet.h"
#include <cassert>
#include <iostream>

int main() {
    using namespace armatron;
    auto framed = [](const std::string& p) { return std::string(1, '\xFA') + p + '\xFB'; };
    Command c;
    for (const auto& item : {std::make_pair("safety_stop", CommandKind::stop),
                             std::make_pair("safety_reset", CommandKind::reset),
                             std::make_pair("connect", CommandKind::connect)}) {
        auto packet = framed(item.first);
        assert(decode_control_packet(packet.data(), packet.size(), c));
        assert(c.kind == item.second);
    }
    auto packet = framed("whl 0.3 -0.2 0.1");
    assert(decode_control_packet(packet.data(), packet.size(), c));
    assert(c.kind == CommandKind::motion && c.x == 0.3 && c.y == -0.2 && c.yaw == 0.1);
    for (auto payload : {"whl nan 0 0", "whl inf 0 0", "whl 1junk 0 0", "whl 0 0", "safety_stop extra"}) {
        auto bad = framed(payload);
        assert(!decode_control_packet(bad.data(), bad.size(), c));
    }
    assert(!decode_control_packet("", 0, c));
    assert(!decode_control_packet("safety_stop", 11, c));
    auto full = framed(std::string(1022, 'x'));
    assert(!decode_control_packet(full.data(), full.size(), c));
    std::cout << "Control packet regression tests passed\n";
}
