#include "udp_control.h"

UDPControl::UDPControl(WheelDriver *driver, int port)
{
    this->driver = driver;
    this->port = port;

    memset(&recv_addr, 0, sizeof(recv_addr));
    last_command_ms.store(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

void UDPControl::start()
{
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        printf("Failed to create socket!\n");

    struct timeval tv;
    tv.tv_sec = 1;  // Set timeout to 5 seconds
    tv.tv_usec = 0; // Not used, but should be set to 0

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv) < 0)
        perror("Cannot set socket options");

    sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    int res = bind(sock, (const sockaddr *)&servaddr, sizeof(servaddr));
    if (res < 0)
        printf("Cannot bimnd to port %d!", port);

    printf("Starting UDP threads!\n");
    send_thread = new thread(&UDPControl::send_thread_loop, this);
    recv_thread = new thread(&UDPControl::recv_thread_loop, this);
}

void UDPControl::stop()
{
    stop_flag = true;
    send_thread->join();
    recv_thread->join();

    delete send_thread;
    delete recv_thread;
}

void UDPControl::send_thread_loop()
{
    while (!stop_flag)
    {
        const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        if (!command_timed_out.load() && now_ms - last_command_ms.load() > 500) {
            driver->stop_for_command_timeout();
            command_timed_out.store(true);
        }
        send_telemetry();

        sleep_for(milliseconds(100));
    }
}

void UDPControl::recv_thread_loop()
{
    while (!stop_flag)
    {
        recv();

        sleep_for(milliseconds(1));
    }
}

void UDPControl::recv()
{
    socklen_t addr_len = sizeof(recv_addr);

    int len = recvfrom(sock, buffer.data(), buffer.size(), 0, (sockaddr *)&recv_addr, &addr_len);

    if (len <= 0)
        return;

    // printf("Received %d bytes\n", len);

    // char ip_str[INET_ADDRSTRLEN];
    // inet_ntop(AF_INET, &recv_addr.sin_addr, ip_str, INET_ADDRSTRLEN);

    // printf("received from %s:%d", ip_str, ntohs(recv_addr.sin_port));

    buffer[len] = '\0';

    if (buffer[0] != PKT_HEADER || buffer[len - 1] != PKT_FOOTER)
    {
        printf("Invalid packet received.\n");
        return;
    }

    proc_recv();
}

void UDPControl::proc_recv()
{
    std::vector<std::string> args;
    args.push_back(std::string());

    //printf("%s\n", buffer.data());

    for (int i = 1; i < buffer.size() - 1; i++)
    { // remove header and footer
        char c = buffer[i];

        if (c == ' ')
            args.push_back(std::string());

        if (c == '\0')
            break;

        args[args.size() - 1].push_back(c);
    }

    if (args.size() == 4 && args[0] == "whl")
    {
        double x = atof(args[1].c_str());
        double y = atof(args[2].c_str());
        double th = atof(args[3].c_str());

        driver->set_velocity(chassis_speeds_t{x, y, th});
        driver->set_motor_enable(true);
        last_command_ms.store(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
        command_timed_out.store(false);
    }
}

void UDPControl::send_telemetry()
{
    in_addr zero;
    memset(&zero, 0, sizeof(zero));

    if (!memcmp(&recv_addr.sin_addr, &zero, sizeof(zero)))
        return;

    chassis_speeds_t spd = driver->get_velocity();
    chassis_position_t pos = driver->get_position();

    int len = snprintf(nullptr, 0, "pos:%f,%f;spd:%f,%f,%f;", pos.x, pos.y, spd.x, spd.y, spd.th);

    std::unique_ptr<char[]> buf(new char[len]);

    snprintf(buf.get(), len, "pos:%f,%f;spd:%f,%f,%f;", pos.x, pos.y, spd.x, spd.y, spd.th);
    std::string str(buf.get(), buf.get() + len - 1);

    send(str);
}

void UDPControl::send(std::string data)
{
    sockaddr_in send_addr = sockaddr_in(recv_addr);
    send_addr.sin_port = htons(port + 1);

    std::unique_ptr<char[]> buf(new char[data.size() + 2]);

    memcpy(buf.get() + 1, data.c_str(), data.length());
    buf[0] = PKT_HEADER;
    buf[data.length() + 1] = PKT_FOOTER;

    sendto(sock, buf.get(), data.size() + 2, 0, (sockaddr *)&send_addr, sizeof(recv_addr));
}
