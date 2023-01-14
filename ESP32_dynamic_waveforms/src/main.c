//Working code from esp-if showing how to use a timer for callbacks. 
//This clock has microsecond resolution but it appears to be no 
//better than 10 usec (before output glitches and errors start happening). 

//URL of the original code
//https://github.com/espressif/esp-idf/blob/8a7f6af6256f4c954053e08e819f55b93ddc1ff1/examples/system/esp_timer/main/esp_timer_example_main.c

/* esp_timer (high resolution timer) example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.

// -------- math --------

The following equation represents a wave that oscillates sinusoidally 
with a frequency of f while its amplitude decays exponentially over time 
with a decay constant of a.

y(t) = A * e^(-at) * sin(2πft + φ)

where:

y(t) is the amplitude of the wave at time t
A is the initial amplitude of the wave
e is the base of the natural logarithm (approximately 2.718)
a is the decay constant, which determines how quickly the amplitude of the wave decays over time
t is time
f is the frequency of the wave in cycles per second
π (pi) is the mathematical constant (approximately 3.14)
φ is the phase angle in radians

*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "sdkconfig.h"

#include "driver/dac.h"
#include "driver/timer.h"
#include "clk.h"
#include <stdio.h>
#include <math.h>



#define VERSION         1      //used to switch from v0 (old) to v1 (new) code

int callbackInMicroseconds = 100;

#if VERSION == 1


// holds the variables needed by the waveform equation
// y(t) = A * e^(-at) * sin(2πft + φ)
struct waveform {
    // f - frequency in cycles/second
    float frequency;    
    // A - amplitude between 0.0 and 1.0
    float amplitude;
    // φ - phase_angle in radians
    float phase_angle;
    // a - controls the waveform decay rate
    float decay;
};

struct waveform waveforms[];
int waveCount;

void setupWaveforms(){

    waveCount = 1;

    //define several waveforms
    struct waveform w1;
    w1.frequency = 10.0;
    w1.amplitude = 0.8;
    w1.phase_angle = 1.57;
    w1.decay = 0.1;

    waveforms[0] = w1;

    // struct waveform w2;
    // w2.frequency = 100.0;
    // w2.amplitude = 0.2;
    // w2.phase_angle = 3.14;
    // w2.decay = 0.2;

    // waveforms[1] = w2;
}
#endif

/*
* This function takes an integer argument, the divider value, 
* which will be used to divide the APB clock. With this function, 
* you can control the time period of the timer by changing the 
* divider value.
*/
void esp_timer_divider_set(int divider) {
    TIMERG0.hw_timer[0].config.divider = divider;
}

static void periodic_timer_callback(void* arg);
// static void oneshot_timer_callback(void* arg);

static const char* TAG = "example";

void app_main(void)
{
    #if VERSION == 1
    setupWaveforms();
    #endif

    dac_output_enable(DAC_CHANNEL_1);

    /* Create two timers:
     * 1. a periodic timer which will run every 0.5s, and print a message
     * 2. a one-shot timer which will fire after 5s, and re-start periodic
     *    timer with period of 1s.
     */

    // esp_timer_divider_set(80);

    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_timer_callback,
            /* name is optional, but may help identify the timer when debugging */
            .name = "periodic"
    };

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    /* The timer has been created but is not running yet */

    // const esp_timer_create_args_t oneshot_timer_args = {
    //         .callback = &oneshot_timer_callback,
    //         /* argument specified here will be passed to timer callback function */
    //         .arg = (void*) periodic_timer,
    //         .name = "one-shot"
    // };
    // esp_timer_handle_t oneshot_timer;
    // ESP_ERROR_CHECK(esp_timer_create(&oneshot_timer_args, &oneshot_timer));


    /* Start the timers */
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, callbackInMicroseconds));  //500,000 microseconds (1/2 sec)
    // ESP_ERROR_CHECK(esp_timer_start_once(oneshot_timer, 5000000));      //5,000,000 microseconds (5 secs)
    ESP_LOGI(TAG, "Started timers, time since boot: %lld us", esp_timer_get_time());

    /* Print debugging information about timers to console every 2 seconds */
    // for (int i = 0; i < 5; ++i) {
    //     ESP_ERROR_CHECK(esp_timer_dump(stdout));
    //     usleep(2000000);
    // }

    // /* Timekeeping continues in light sleep, and timers are scheduled
    //  * correctly after light sleep.
    //  */
    // int64_t t1 = esp_timer_get_time();
    // ESP_LOGI(TAG, "Entering light sleep for 0.5s, time since boot: %lld us", t1);

    // ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(500000));
    // esp_light_sleep_start();

    // int64_t t2 = esp_timer_get_time();
    // ESP_LOGI(TAG, "Woke up from light sleep, time since boot: %lld us", t2);

    // assert(llabs((t2 - t1) - 500000) < 1000);

    // /* Let the timer run for a little bit more */
    // usleep(2000000);    //2,000,000 microseconds (2 secs)

    // /* Clean up and finish the example */
    // ESP_ERROR_CHECK(esp_timer_stop(periodic_timer));
    // ESP_ERROR_CHECK(esp_timer_delete(periodic_timer));
    // ESP_ERROR_CHECK(esp_timer_delete(oneshot_timer));
    // ESP_LOGI(TAG, "Stopped and deleted timers");
}

static void periodic_timer_callback(void* arg)
{
    int64_t time_since_boot = esp_timer_get_time(); //microseconds
    double _t_ = time_since_boot/1000000.0;
    // ESP_LOGI(TAG, "Periodic timer called, time since boot: %lld us", time_since_boot);

#if VERSION == 1

    // ------------- START new way not working --------------
    float y = 0;  //the final value to be sent to the ESP32 output pin (between 0 and 255)
    for (int i=0; i<waveCount; i++){
        struct waveform w = waveforms[i];
        // y(t) = A * e^(-at) * sin(2πft + φ)
        float exponential = pow(M_E, (-1) * w.decay * _t_);
        float angle = M_TWOPI * w.frequency * _t_ + w.phase_angle;
        //sum all the y values as floats in order to maintain resolution during the calculations
        y = y + ( w.amplitude * exponential * ( 127.0 + ( 127.0 * sin(angle) )));
    }
    dac_output_voltage(DAC_CHANNEL_1, ((int)y));
    // ------------- END new way not working --------------

#elif VERSION == 0

//    -------- old way that was working ----------
    //data for each waveform (frequency, amplitude, phase angle, attenuation)
    float wave1[] = {100.0, 0.5, 0.0, 0.5};
    float wave2[] = {1000.0, 0.1, 0.0, 0.5};

    // ESP_LOGI(TAG, "Periodic timer called, time since boot: %lld us", time_since_boot);

    // formula for a sine wave with frequency (ω) amplitude (A) and phase angle (φ)
    //   f(t) = A sin(ωt + φ)    
    float waveform1 = (wave1[3] * ( ((float)127) + ( ((float)127) * (wave1[1] * (sin(M_TWOPI * wave1[0] * _t_ + wave1[2])))))); 
    float waveform2 = (wave2[3] * ( ((float)127) + ( ((float)127) * (wave2[1] * (sin(M_TWOPI * wave2[0] * _t_ + wave2[2])))))); 
    // printf("%.9f\t", now);
    // printf("%.3f\t", waveform1);
    // printf("%.3f\t", waveform2);
    int output = (int)(waveform1 + waveform2);
    // printf("%d\n", output);
    
    dac_output_voltage(DAC_CHANNEL_1, output);

#endif

}

// static void oneshot_timer_callback(void* arg)
// {
//     int64_t time_since_boot = esp_timer_get_time();
//     ESP_LOGI(TAG, "One-shot timer called, time since boot: %lld us", time_since_boot);
//     esp_timer_handle_t periodic_timer_handle = (esp_timer_handle_t) arg;
//     /* To start the timer which is running, need to stop it first */
//     ESP_ERROR_CHECK(esp_timer_stop(periodic_timer_handle));
//     ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer_handle, 1000000));
//     time_since_boot = esp_timer_get_time();
//     ESP_LOGI(TAG, "Restarted periodic timer with 1s period, time since boot: %lld us",
//             time_since_boot);
// }

