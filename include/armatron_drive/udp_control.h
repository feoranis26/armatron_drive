#ifndef UDP_CONTROL_H
#define UDP_CONTROL_H

#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <bits/stdc++.h> 
#include <stdlib.h>
#include <atomic>
#include <cstdint>

#include "whl_driver.h"

#define MAX_BUFFER 1024
#define PKT_HEADER 0xFA
#define PKT_FOOTER 0xFB

class UDPControl {
public:
    UDPControl(WheelDriver* driver, int port);
    void start();
    void stop();
private:
    void send_thread_loop();
    void recv_thread_loop();

    void recv();
    void proc_recv();

    void send_telemetry();
    void send(std::string data);

    int sock;
    int port;

    bool connected = false;
    std::atomic<bool> stop_flag{false};
    std::atomic<bool> command_timed_out{false};
    std::atomic<std::int64_t> last_command_ms{0};

    thread *send_thread = nullptr;
    thread *recv_thread = nullptr;

    WheelDriver* driver;

    sockaddr_in recv_addr;

    std::vector<char> buffer = std::vector<char>(MAX_BUFFER);
};

#endif
