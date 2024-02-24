#include <stdio.h>

#include "contiki.h"
#include "sys/rtimer.h"
#include "board-peripherals.h"
#include "buzzer.h"

#include <stdint.h>

PROCESS(process_rtimer, "RTimer");
AUTOSTART_PROCESSES(&process_rtimer);

static int curr_lux;
static int curr_mpu_x;
static int curr_mpu_y;
static int curr_mpu_z;

static int state = 0;
static int counter = 4;

static struct rtimer timer_rtimer;
static rtimer_clock_t state_start_time;
static rtimer_clock_t SAMPLE_TIME = RTIMER_SECOND / 4;
static rtimer_clock_t BUZZ_TIME = RTIMER_SECOND * 2;
static rtimer_clock_t WAIT_TIME = RTIMER_SECOND * 2;
int buzzerFrequency[8] = {2093, 2349, 2637, 2794, 3156, 3520, 3951, 4186}; // notes on a piano

/*---------------------------------------------------------------------------*/
static void init_opt_reading(void);
static int get_light_reading(void);
static int check_change_in_lux(void);
static void init_mpu_reading(void);
static int check_change_in_mpu(void);
static void transition_to_idle(void);
static void transition_to_buzz(void);
static void transition_to_wait(void);
void do_rtimer_timeout(struct rtimer *timer, void *ptr);
/*---------------------------------------------------------------------------*/

static void
init_opt_reading(void)
{
  SENSORS_ACTIVATE(opt_3001_sensor);
}

static void
init_mpu_reading(void)
{
  mpu_9250_sensor.configure(SENSORS_ACTIVE, MPU_9250_SENSOR_TYPE_ALL);
}

static int
get_light_reading()
{
  int value;

  value = opt_3001_sensor.value(0);
  if (value != CC26XX_SENSOR_READING_ERROR)
  {
    printf("OPT: Light=%d.%02d lux\n", value / 100, value % 100);
    init_opt_reading();
    return value;
  }
  else
  {
    printf("OPT: Light Sensor's Warming Up\n\n");
    init_opt_reading();
    return -1;
  }
}

static int
check_change_in_lux()
{
  int new_lux = get_light_reading();
  if (new_lux != -1) {
    if (abs(new_lux - curr_lux) > 300) {
      printf("The light level has changed from %d to %d\n", curr_lux, new_lux);
      curr_lux = new_lux;
      return 1;
    } else {
      // printf("The light level has not changed\n");
      curr_lux = new_lux;
      return 0;
    }
  } else {
    printf("WTF has happened!??@!\n");
    return 0;
  }
}

// Check if there is a significant change in the MPU reading
// on all three axes, update the current reading regardless
// Return 1 if there is a significant change, 0 otherwise
static int
check_change_in_mpu()
{
  int significant_change = 1000;

  int new_mpu_x = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_X);
  int new_mpu_y = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Y);
  int new_mpu_z = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Z);

  if (abs(new_mpu_x - curr_mpu_x) > significant_change) {
    printf("The mpu_x has changed from %d to %d\n", curr_mpu_x, new_mpu_x);
    curr_mpu_x = new_mpu_x;
    init_mpu_reading();
    return 1;
  } else if (abs(new_mpu_y - curr_mpu_y) > significant_change) {
    printf("The mpu_y has changed from %d to %d\n", curr_mpu_y, new_mpu_y);
    curr_mpu_y = new_mpu_y;
    init_mpu_reading();
    return 1;
  } else if (abs(new_mpu_z - curr_mpu_z) > significant_change) {
    printf("The mpu_z has changed from %d to %d\n", curr_mpu_z, new_mpu_z);
    curr_mpu_z = new_mpu_z;
    init_mpu_reading();
    return 1;
  } else {
    curr_mpu_x = new_mpu_x;
    curr_mpu_y = new_mpu_y;
    curr_mpu_z = new_mpu_z;
    init_mpu_reading();
    return 0;
  }
}

static void transition_to_idle(void) {
    printf("Transitioning to idle...\n");
    state = 0;

}

static void transition_to_buzz(void) {
    printf("Transitioning to buzz...\n");
    state = 1;

}

static void transition_to_wait(void) {
    printf("Transitioning to wait...\n");
    state = 2;

}

static void update_lux() {
    printf("Updating lux\n");
    curr_lux = get_light_reading();
}

static void update_mpu() {
    printf("Updating mpu\n");
    curr_mpu_x = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_X);
    curr_mpu_y = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Y);
    curr_mpu_z = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Z);
    init_mpu_reading();
}

void do_rtimer_timeout(struct rtimer *timer, void *ptr)
{
    rtimer_clock_t current_time = RTIMER_NOW();
    rtimer_clock_t state_duration_ticks;
    rtimer_clock_t state_duration_seconds;

    switch(state) {
        // IDLE state
        case 0:
            counter = 4;

            if (check_change_in_lux() || check_change_in_mpu()){
                transition_to_buzz();
                // update_mpu();
            }
            else {
                rtimer_set(&timer_rtimer, current_time + SAMPLE_TIME, 0, do_rtimer_timeout, NULL);
            }
            
            // Calculate state duration
            state_duration_ticks = current_time - state_start_time;
            state_duration_seconds = state_duration_ticks / RTIMER_SECOND;
            
            // Print state duration
            printf("State: IDLE, Duration: %lu ticks (%lu seconds)\n", state_duration_ticks, state_duration_seconds);

            break;
        
        // BUZZ state
        case 1:
            buzzer_start(buzzerFrequency[counter]);
            rtimer_set(&timer_rtimer, current_time + BUZZ_TIME, 0, do_rtimer_timeout, NULL);
            transition_to_wait();
            
            // Calculate state duration
            state_duration_ticks = current_time - state_start_time;
            state_duration_seconds = state_duration_ticks / RTIMER_SECOND;
            
            // Print state duration
            printf("State: BUZZ, Duration: %lu ticks (%lu seconds)\n", state_duration_ticks, state_duration_seconds);
            
            // Update state start time
            state_start_time = current_time;
            break;

        // WAIT state
        case 2:
            buzzer_stop();
            if (--counter > 0) {
                rtimer_set(&timer_rtimer, current_time + WAIT_TIME, 0, do_rtimer_timeout, NULL);
                transition_to_buzz();
            } else {
                rtimer_set(&timer_rtimer, current_time + WAIT_TIME, 0, do_rtimer_timeout, NULL);
                update_lux();
                update_mpu();
                transition_to_idle();
            }
            
            // Calculate state duration
            state_duration_ticks = current_time - state_start_time;
            state_duration_seconds = state_duration_ticks / RTIMER_SECOND;
            
            // Print state duration
            printf("State: WAIT, Duration: %lu ticks (%lu seconds)\n", state_duration_ticks, state_duration_seconds);

            // Update state start time
            state_start_time = current_time;
            break;
    }

}


PROCESS_THREAD(process_rtimer, ev, data)
{
  PROCESS_BEGIN();
  init_opt_reading();
  init_mpu_reading();
  state_start_time = RTIMER_NOW();

  while (1)
  {

    rtimer_set(&timer_rtimer, RTIMER_NOW() + SAMPLE_TIME, 0, do_rtimer_timeout, NULL);

    PROCESS_YIELD();
  }

  PROCESS_END();
}
