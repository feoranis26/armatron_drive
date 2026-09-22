#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <pigpio.h>
#include <vector>
#include <chrono>
#include <thread>
#include <csignal>

#include "stepper.h"
#include "whl_driver.h"
#include "udp_control.h"
#include "stepper.h"

using namespace std;

StepperWaveformTransmitter *tx;
WheelDriver *driver;
UDPControl *udp;

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

    tx->stop();
    driver->stop();
    udp->stop();

    gpioTerminate();

    delete driver;
    delete tx;
    delete udp;
}


void fatal(int sg)
{
    stop();

    exit(EXIT_FAILURE);
}


int main()
{
    int cfg = gpioCfgGetInternals();
    cfg |= PI_CFG_NOSIGHANDLER; // (1<<10)
    gpioCfgSetInternals(cfg);

    signal(SIGTERM, fatal);
    signal(SIGSEGV, fatal);
    signal(SIGABRT, fatal);
    signal(SIGINT, fatal);

    startup();

    while (true)
    {
        sleep_for(seconds(1));
    }
}