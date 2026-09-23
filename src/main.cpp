#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <pigpio.h>
#include <vector>
#include <chrono>
#include <thread>
#include <csignal>
#include <cassert>

#include "whl_driver.h"
#include "udp_control.h"
#include "stepper.h"

using namespace std;

StepperWaveformTransmitter *tx;
WheelDriver *driver;
UDPControl *udp;
volatile std::sig_atomic_t shutdown_requested = 0;

void startup()
{
    printf("Starting up\n");
    // int gpio = pigpio_start(NULL, NULL);
    int gpio = gpioInitialise();
    assert(gpio >= 0);

    gpioWaveClear();
    gpioWaveTxStop();

    motor_config_t fl(12, 6);
    motor_config_t fr(16, 11);
    motor_config_t br(20, 19);
    motor_config_t bl(21, 26);
    //motor_config_t bl(26, 16);

    motor_config_t motors[]{fl, fr, br, bl};

    driver_config_t config{
        5.0,
        1.0,
        3183*6,
        5};

    tx = new StepperWaveformTransmitter(gpio, 10000, 5);

    driver = new WheelDriver(gpio, tx, motors, config);
    udp = new UDPControl(driver, 11753);

    tx->start();
    driver->start();
    udp->start();

    driver->set_velocity(chassis_speeds_t{0.1, 0.0, 0.0});
    sleep_for(milliseconds(100));
    driver->set_velocity(chassis_speeds_t{-1.0, 0.0, 0.0});
    sleep_for(milliseconds(100));
    driver->set_velocity(chassis_speeds_t{1.0, 0.0, 0.0});
    sleep_for(milliseconds(100));
    driver->set_velocity(chassis_speeds_t{-1.0, 0.0, 0.0});
    sleep_for(milliseconds(100));
    driver->set_velocity(chassis_speeds_t{0.0, 0.0, 0.0});
    sleep_for(milliseconds(100));
    driver->set_motor_enable(true);
}

void stop()
{
    printf("Shutting down\n");

    // Stop ingress first, then deassert motor enable before stopping the
    // waveform generator. This is normal-thread code, never a signal handler.
    if (udp != nullptr)
        udp->stop();
    if (driver != nullptr)
        driver->stop();
    if (tx != nullptr)
        tx->stop();

    gpioTerminate();

    delete udp;
    delete driver;
    delete tx;

    udp = nullptr;
    driver = nullptr;
    tx = nullptr;
}

void request_shutdown(int)
{
    shutdown_requested = 1;
}


int main()
{
    int cfg = gpioCfgGetInternals();
    cfg |= PI_CFG_NOSIGHANDLER; // (1<<10)
    gpioCfgSetInternals(cfg);

    signal(SIGTERM, request_shutdown);
    signal(SIGINT, request_shutdown);

    startup();

    while (!shutdown_requested)
    {
        sleep_for(seconds(1));
    }

    stop();
    return EXIT_SUCCESS;
}
