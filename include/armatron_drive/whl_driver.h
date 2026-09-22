#ifndef WHL_DRIVER
#define WHL_DRIVER

#include "stepper.h"
#include <chrono>

#define SPEED_EPSILON 0.01

struct chassis_speeds_t
{
    double x;
    double y;
    double th;
};


struct chassis_position_t
{
    double x;
    double y;
};

struct motor_config_t
{
    motor_config_t(unsigned int dir, unsigned int step) {
        dir_pin = dir;
        step_pin = step;
    }
    unsigned int dir_pin;
    unsigned int step_pin;
};

struct driver_config_t
{
    double acceleration;
    double max_speed;
    double steps_per_mps;

    int en_pin;
};

class WheelDriver
{
public:
    WheelDriver(int gpio, StepperWaveformTransmitter *tx, motor_config_t *configs, driver_config_t config);
    ~WheelDriver();
    void start();
    void stop();

    void set_velocity(chassis_speeds_t speeds);
    chassis_speeds_t get_velocity();
    chassis_position_t get_position();

    void set_motor_enable(bool enabled);

private:
    void thread_loop();
    StepperWaveformTransmitter *tx;
    Stepper *fl, *fr, *br, *bl;
    driver_config_t config;

    thread *thr = nullptr;
    bool stop_flag = false;

    bool motor_en = false;

    chassis_speeds_t tgt_speed;
    std::chrono::_V2::system_clock::time_point last_motion;
};
#endif