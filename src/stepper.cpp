#include "stepper.h"

us_t time_us()
{
    struct timeval tv;
    gettimeofday(&tv, 0);

    return (us_t)(((double)tv.tv_sec * 1000000) + (double)tv.tv_usec);
}

Stepper::Stepper(int gpio, int dir, int step, double accel, double max_spd)
{
    this->gpio = gpio;
    pin_dir = dir;
    pin_step = step;

    acc = accel;
    max_speed = max_spd;

    last_step_us = time_us();
    last_speed_update_us = time_us();

    gpioSetMode(pin_step, PI_OUTPUT);
    gpioSetMode(pin_dir, PI_OUTPUT);

    gpioWrite(pin_step, 0);
    gpioWrite(pin_dir, 0);
}

void Stepper::wait_motion_end()
{
    while (position != target)
        sleep_for(milliseconds(10));
}

void Stepper::wait_speed_reached()
{
    while (abs(speed - target_speed) > 5)
        sleep_for(milliseconds(10));
}

us_t Stepper::_get_next_state_change_us()
{
    // if(position == target && isnan(target_speed))
    //     return 0;

    // double dist_since_start = 0.5 * (speed*speed / acc);
    // double needed_speed = sqrt(2*acc*dist_since_start);

    if (abs(speed) <= 1.0)
        return 0;

    us_t us_per_step = (us_t)(1000000.0 / abs(speed));
    return us_per_step + last_step_us;
}

void Stepper::_update_speed(us_t current_us)
{
    double time_delta = (current_us - last_speed_update_us) / 1000000.0;
    last_speed_update_us = current_us;

    if (!isnan(target_speed))
    {
        if (last_dir == 2)
        {
            gpioWrite(pin_dir, speed < 0);
            last_dir = speed < 0;
        }

        if(abs(target_speed - speed) < 2) {
            speed = target_speed;
            return;
        }
        
        if (speed > target_speed)
            speed -= acc * time_delta;
        else if (speed < target_speed)
            speed += acc * time_delta;

        return;
    }

    if (position == target)
    {
        speed = 0;
        return;
    }

    double dist_to_stop = (speed * speed / (acc * 2)) * (speed > 0 ? 1 : -1);

    if (position + dist_to_stop > target)
        speed -= acc * time_delta;
    else
        speed += acc * time_delta;

    speed = min(max(-max_speed, speed), max_speed);
}

step_pulse_t Stepper::_step_now(us_t current_us)
{
    last_step_us = current_us;

    step_pulse_t pulse;
    pulse.up = 0;
    pulse.down = 0;

    if (state == 0)
        pulse.up |= 1 << pin_step;
    else if (state == 1)
        pulse.down |= 1 << pin_step;

    state = !state;

    if (speed < 0 && last_dir == 1)
        pulse.up |= 1 << pin_dir;
    else if (speed > 0 && last_dir == 0)
        pulse.down |= 1 << pin_dir;

    last_dir = speed > 0;

    if (speed != 0)
        position += speed > 0 ? 1 : -1;

    if (position == target && isnan(target_speed))
    {
        speed = 0;
        return pulse;
    }

    return pulse;
}

StepperWaveformTransmitter::StepperWaveformTransmitter(int gpio, us_t plan_length_us, int enable_p)
{
    planning_len = plan_length_us;

    this->gpio = gpio;
    current_time = time_us();

    // pulses = (gpioPulse_t *)malloc(sizeof(gpioPulse_t) * MAX_PULSES);

    // waves.assign(2, PI_NO_TX_WAVE);
}

StepperWaveformTransmitter::~StepperWaveformTransmitter()
{
}

void StepperWaveformTransmitter::add_stepper(Stepper *stepper)
{
    steppers.push_back(stepper);
}

void StepperWaveformTransmitter::start()
{
    printf("Starting thread!\n");
    printf("This: %p", this);
    gen_thread = new thread(&StepperWaveformTransmitter::thread_loop, this);
}

void StepperWaveformTransmitter::stop()
{
    printf("Signalling thread to stop!\n");
    stop_flag = true;
    gen_thread->join();
    
    delete gen_thread;
}

wf_id_t StepperWaveformTransmitter::create_and_transmit()
{
    next_wf_t next_wf = try_create_wf(current_time);
    int res = gpioWaveTxSend(next_wf.next_wf_id, PI_WAVE_MODE_ONE_SHOT_SYNC);

    // if (res < 0)
    // printf("error: %s\n", pigpio_error(res));
    assert(res >= 0); // Failed to transmit wf!

    current_time += planning_len;
    return next_wf.next_wf_id;
}

wf_id_t StepperWaveformTransmitter::wait_tx_end(wf_id_t wait_wave)
{
    //printf("Wait until %d ends\n", wait_wave);

    while (gpioWaveTxAt() == wait_wave)
        sleep_for(microseconds(planning_len / 10));

    sleep_for(microseconds(planning_len / 10));

    return wait_wave;
}

void StepperWaveformTransmitter::wait_and_fill_buffer()
{
    /*wf_id_t check_wave = gpioWaveTxAt();
    printf("Current wave=%d\n", check_wave);
    if (check_wave != PI_NO_TX_WAVE)
    {
        size_t index = 0;
        for (int i = 0; i < waves.size(); i++)
            if (waves[i] == check_wave)
                index = i;

        if (index != 0)
            printf("Underrun!!!\n");
        else
            wait_tx_end();

        wf_id_t check_wave = gpioWaveTxAt();
        printf("Current wave while deleting wave=%d\n", check_wave);

        /*for (int i = index; i >= 0; i--)
        {
            if (gpioWaveTxAt() == i)
            printf("deleting wave @%d, current num waves: %d\n", i, waves.size());
            gpioWaveDelete(waves[i]);
            //printf("ok, remove from vector\n");
            waves.erase(waves.begin() + i);
            //printf("removed.\n");
        }*/
    /*printf("deleting wave @%d, current num waves: %d\n", waves[0], waves.size());

    if

    int res = gpioWaveDelete(waves[0]);
    if (res != 0)
        printf("res = %d\n", res);

    waves.erase(waves.begin());
}
else{
    printf("TOOL ATER!!!\n");
    gpioWaveClear();
    waves.clear();
}

while (waves.size() < 2) {
    //printf("adding new wave, current num waves: %d\n", waves.size());
    int wave = create_and_transmit();
    printf("Adding wave: %d\n", wave);
    waves.push_back(wave);
}

for (int wave : waves) {
    printf("Result has wave: %d\n", wave);
}*/

    wf_id_t tx = gpioWaveTxAt();

    //printf("Now transmitting: %d, current: %d, next up: %d\n", tx, current, next_up);

    if (tx == PI_NO_TX_WAVE)
    {
        printf("TOO LATE! Resetting waves! \n");
        reset();
        return;
    }

    if (tx == current)
    {
        wait_tx_end(current);
        wf_id_t tx = gpioWaveTxAt();

        shift();
        return;
    }

    if (tx == next_up)
    {
        printf("Underrun!\n");
        shift();
        return;
    }
}

next_wf_t StepperWaveformTransmitter::create_wf(us_t start_us)
{
    us_t current_us = 0;

    int num_pulses = 0;

    for (Stepper *stepper : steppers)
        stepper->_update_speed(start_us);

    us_t last_speed_update_us = start_us;

    while (current_us < planning_len && num_pulses < MAX_PULSES - 1)
    {
        Stepper *stepper_to_step = nullptr;
        us_t next_step_us = INT64_MAX;

        for (Stepper *stepper : steppers)
        {
            us_t step_us = stepper->_get_next_state_change_us();

            if (step_us != 0 && step_us < next_step_us)
            {
                next_step_us = step_us;
                stepper_to_step = stepper;
            }
        }
        if (stepper_to_step == nullptr) // no pulses left for current waveform
            break;

        us_t delta_us = next_step_us - (start_us + current_us);

        if (delta_us < 0)
        {
            //printf("Too late! Should be %lld us earlier\n", -delta_us);

            delta_us = 0;
        }

        if (delta_us > 100) // To ensure that stepper speeds are updated at least once every 1ms
            delta_us = 100;

        current_us += delta_us;

        if (current_us > planning_len)
            break;

        if ((current_us + start_us) - last_speed_update_us > 100)
        {
            for (Stepper *stepper : steppers)
                stepper->_update_speed(current_us + start_us);

            last_speed_update_us = start_us + current_us;
        }

        gpioPulse_t delay_pulse;

        delay_pulse.gpioOn = 0;
        delay_pulse.gpioOff = 0;
        delay_pulse.usDelay = (int)(delta_us / 1); // was 2????
        pulses[num_pulses++] = delay_pulse;

        int pulse_up = 0;
        int pulse_down = 0;

        while (true)
        {
            stepper_to_step = nullptr;

            for (Stepper *stepper : steppers)
            {
                us_t step_us = stepper->_get_next_state_change_us();

                if (step_us <= current_us + start_us && step_us != 0)
                {
                    step_pulse_t stp_pulse = stepper->_step_now(start_us + current_us);

                    pulse_up |= stp_pulse.up;
                    pulse_down |= stp_pulse.down;

                    stepper_to_step = stepper;
                }
            }

            if (stepper_to_step == nullptr)
                break;
        }

        gpioPulse_t full_pulse;
        full_pulse.gpioOn = pulse_up;
        full_pulse.gpioOff = pulse_down;
        full_pulse.usDelay = 0;

        pulses[num_pulses++] = full_pulse;
    }

    if (current_us < planning_len)
    {
        gpioPulse_t pulse;
        pulse.gpioOn = 0;
        pulse.gpioOff = 0;
        pulse.usDelay = planning_len - current_us;

        pulses[num_pulses++] = pulse;
    }

    total_pulses += num_pulses;

    int num_wf = gpioWaveAddGeneric(num_pulses, pulses);

    wf_id_t new_wave = gpioWaveCreatePad(49, 49, 0);

    if (new_wave < 0)
    {
        // printf("error: %s\n", (new_wave));
        printf("error: %d\n", new_wave);
        printf("pulse count: %d\n", num_pulses);
        printf("registered pulse count: %d\n", num_wf);
    }
    // assert(new_wave >= 0); // Wave create failed!

    next_wf_t next_wf;
    next_wf.next_wf_id = new_wave;
    next_wf.current_us = current_us;

    return next_wf;
}

next_wf_t StepperWaveformTransmitter::try_create_wf(us_t start)
{
    next_wf_t wf = create_wf(start);

    if (wf.next_wf_id >= 0)
        return wf;

    int cbs_m = gpioWaveGetMaxCbs();
    int cbs_h = gpioWaveGetHighCbs();
    int micros_r = gpioWaveGetMaxMicros();

    int pulse_m = gpioWaveGetMaxPulses();
    int pulse_h = gpioWaveGetHighPulses();

    printf("Max CBs: %d\n", cbs_m);
    printf("Highest CBs: %d\n", cbs_h);
    printf("Max us: %d\n", micros_r);
    printf("Max pulses: %d\n", pulse_m);
    printf("High pulses: %d\n", pulse_h);
    printf("Total pulses: %d\n", total_pulses);

    sleep_for(microseconds(planning_len / 10));

    gpioWrite(5, 0);
    gpioTerminate();
    gpioInitialise();
}

void StepperWaveformTransmitter::thread_loop()
{
    printf("Thread started.\n");
    printf("This: %p\n", this);
    printf("Pi: %d\n", gpio);

    gpioWaveClear();

    printf("Starting transmit loop!\n");
    while (!stop_flag)
    {
        wait_and_fill_buffer();
    }

    printf("Thread stopped.\n");
}

void StepperWaveformTransmitter::shift()
{
    delete_cur();

    current = next_up;
    next_up = create_and_transmit();
}

void StepperWaveformTransmitter::reset()
{
    printf("Resetting waveforms.\n");

    current = PI_NO_TX_WAVE;
    next_up = PI_NO_TX_WAVE;
    delete_next();

    int result = gpioWaveClear();

    if (result < 0)
        printf("Could not reset waveforms!!.\n");

    current = create_and_transmit();
    next_up = create_and_transmit();
}

void StepperWaveformTransmitter::delete_cur()
{
    int result = 0;
    if (current != PI_NO_TX_WAVE)
        result = gpioWaveDelete(current);

    if (result != 0)
        printf("Could not delete current!\n");

    current = PI_NO_TX_WAVE;
}

void StepperWaveformTransmitter::delete_next()
{
    int result = 0;
    if (next_up != PI_NO_TX_WAVE)
        result = gpioWaveDelete(next_up);

    if (result != 0)
        printf("Could not delete next up!\n");

    next_up = PI_NO_TX_WAVE;
}
