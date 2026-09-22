#ifndef STEPPER_H
#define STEPPER_H

#define MAX_PULSES 12000

#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <pigpio.h>
#include <vector>
#include <chrono>
#include <thread>
#include <math.h>
#include <cassert>

using namespace std;
using namespace std::this_thread;
using namespace std::chrono_literals;
using namespace std::chrono;
using std::chrono::system_clock;

typedef long long us_t;
typedef int wf_id_t;

us_t time_us();

struct step_pulse_t
{
    int up;
    int down;
};

struct next_wf_t
{
    wf_id_t next_wf_id;
    us_t current_us;
};

class Stepper
{
public:
    Stepper(int gpio, int dir, int step, double accel, double max_spd);

    double speed = 0.0;
    double acc = 0.0;
    double max_speed = 0.0;

    int position = 0;          // current position of stepper
    int target = 0;            // target position to move stepper
    double target_speed = NAN; // hold constant speed, overrides target

    void wait_motion_end();
    void wait_speed_reached();

    us_t _get_next_state_change_us(); // these are called by waveform transmitter, not for general use
    void _update_speed(us_t current_us);
    step_pulse_t _step_now(us_t current_us);

private:
    int pin_dir = 0;
    int pin_step = 0;

    us_t last_step_us = 0;
    us_t last_speed_update_us = 0;

    int last_dir = 2;
    int state = 0;

    int gpio = 0;
};

class StepperWaveformTransmitter
{
public:
    StepperWaveformTransmitter(int gpio, us_t plan_length_us, int enable_p);
    ~StepperWaveformTransmitter();
    void add_stepper(Stepper *stepper);
    void start();
    void stop();
    next_wf_t create_wf(us_t start);
    next_wf_t try_create_wf(us_t start);
    wf_id_t create_and_transmit();
    wf_id_t wait_tx_end(wf_id_t wait_wave);
    void wait_and_fill_buffer();

private:
    void thread_loop();

    void shift();
    void reset();

    void delete_cur();
    void delete_next();
    
    int gpio = 0;

    us_t planning_len = 0;
    us_t current_time = 0;

    vector<Stepper *> steppers;
    
    wf_id_t current = PI_NO_TX_WAVE;
    wf_id_t next_up = PI_NO_TX_WAVE;

    thread *gen_thread = nullptr;

    bool stop_flag = false;

    gpioPulse_t pulses[MAX_PULSES];

    int total_pulses = 0;
};

#endif