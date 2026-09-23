#include "whl_driver.h"

WheelDriver::WheelDriver(int gpio, StepperWaveformTransmitter *tx, motor_config_t *configs, driver_config_t config)
{
    this->tx = tx;

    this->config = config;

    int acceleration_steps = config.acceleration * config.steps_per_mps;
    int velocity_steps = config.max_speed * config.steps_per_mps;

    fl = new Stepper(gpio, configs[0].dir_pin, configs[0].step_pin, acceleration_steps, velocity_steps);
    fr = new Stepper(gpio, configs[1].dir_pin, configs[1].step_pin, acceleration_steps, velocity_steps);
    br = new Stepper(gpio, configs[2].dir_pin, configs[2].step_pin, acceleration_steps, velocity_steps);
    bl = new Stepper(gpio, configs[3].dir_pin, configs[3].step_pin, acceleration_steps, velocity_steps);

    tx->add_stepper(fl);
    tx->add_stepper(fr);
    tx->add_stepper(br);
    tx->add_stepper(bl);
    last_motion = std::chrono::high_resolution_clock::now();
}

WheelDriver::~WheelDriver()
{
    delete fl;
    delete fr;
    delete br;
    delete bl;
}

void WheelDriver::start()
{
    stop_flag = false;
    thr = new thread(&WheelDriver::thread_loop, this);
}

void WheelDriver::stop()
{
    // The driver-enable line is active high.  Deassert it before waiting for
    // worker threads so a terminating process cannot leave the motors powered.
    gpioWrite(config.en_pin, 0);

    stop_flag = true;
    thr->join();
    
    delete thr;
}

void WheelDriver::set_velocity(chassis_speeds_t speeds)
{
    printf("Set velocity: %f, %f, %f\n", speeds.x, speeds.y, speeds.th);

    double x = speeds.x * config.steps_per_mps;
    double y = speeds.y * config.steps_per_mps;
    double th = speeds.th * config.steps_per_mps;

    fl->target_speed = -(x + y + th);
    fr->target_speed = (x - y - th);
    br->target_speed = (x + y - th);
    bl->target_speed = -(x - y + th);

    tgt_speed = speeds;
}

chassis_speeds_t WheelDriver::get_velocity()
{
    double fl_s = (fl->speed / config.steps_per_mps) / 4.0;
    double fr_s = (-fr->speed / config.steps_per_mps) / 4.0;
    double br_s = (-br->speed / config.steps_per_mps) / 4.0;
    double bl_s = (bl->speed / config.steps_per_mps) / 4.0;

    chassis_speeds_t speeds;
    speeds.x = fl_s + fr_s + br_s + bl_s;
    speeds.y = fl_s - fr_s + br_s - bl_s;
    speeds.th = fl_s - fr_s - br_s + bl_s;

    return speeds;
}

chassis_position_t WheelDriver::get_position()
{
    double fl_p = (fl->position / config.steps_per_mps) / 4.0;
    double fr_p = (-fr->position / config.steps_per_mps) / 4.0;
    double br_p = (-br->position / config.steps_per_mps) / 4.0;
    double bl_p = (bl->position / config.steps_per_mps) / 4.0;

    chassis_position_t position;
    position.x = fl_p + fr_p + br_p + bl_p;
    position.y = fl_p - fr_p + br_p - bl_p;

    return position;
}

void WheelDriver::set_motor_enable(bool enabled)
{
    motor_en = enabled;
}

void WheelDriver::stop_for_command_timeout()
{
    set_velocity(chassis_speeds_t{0.0, 0.0, 0.0});
    set_motor_enable(false);
}

void WheelDriver::thread_loop()
{
    while (!stop_flag)
    {
        sleep_for(milliseconds(10));
        //printf("Target speed (steps/s): fl=%f, fr=%f, br=%f, bl=%f\n", fl->target_speed, fr->target_speed, br->target_speed, bl->target_speed);

        if (abs(get_velocity().x) + abs(get_velocity().y) + abs(get_velocity().th) > SPEED_EPSILON)
            last_motion = std::chrono::high_resolution_clock::now();

        auto now = std::chrono::high_resolution_clock::now();
        if (!motor_en || std::chrono::duration<double, std::milli>(now-last_motion).count() > 2000) {
            printf("Motor disabled\n");
            gpioWrite(config.en_pin, 0);
        }
        else {
            printf("Motor enabled\n");
            gpioWrite(config.en_pin, 1);
        }
    }
}
