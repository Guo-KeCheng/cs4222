#include <stdio.h>

#include "contiki.h"
#include "sys/rtimer.h"
#include "board-peripherals.h"
#include "buzzer.h"

#include <stdint.h>

PROCESS(process_rtimer, "RTimer");
AUTOSTART_PROCESSES(&process_rtimer);

static int curr_lux;
static int curr_mpu;

// static int counter_rtimer;
static struct rtimer timer_rtimer;
static rtimer_clock_t timeout_rtimer = RTIMER_SECOND / 4;  
int buzzerFrequency[8]={2093,2349,2637,2794,3156,3520,3951,4186}; // hgh notes on a piano

/*---------------------------------------------------------------------------*/
static void init_opt_reading(void);
static int get_light_reading(void);
// static void print_mpu_reading(int reading);
// static void get_mpu_reading(void);
static int check_change_in_lux(void);
static int check_change_in_mpu(void);
/*---------------------------------------------------------------------------*/


// static void
// get_mpu_reading()
// {
//   int value;

//   printf("MPU Gyro: X=");
//   value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_X);
//   print_mpu_reading(value);
//   printf(" deg/sec\n");

//   printf("MPU Gyro: Y=");
//   value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Y);
//   print_mpu_reading(value);
//   printf(" deg/sec\n");

//   printf("MPU Gyro: Z=");
//   value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Z);
//   print_mpu_reading(value);
//   printf(" deg/sec\n");

//   printf("MPU Acc: X=");
//   value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_X);
//   print_mpu_reading(value);
//   printf(" G\n");

//   printf("MPU Acc: Y=");
//   value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Y);
//   print_mpu_reading(value);
//   printf(" G\n");

//   printf("MPU Acc: Z=");
//   value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Z);
//   print_mpu_reading(value);
//   printf(" G\n");
// }

void
do_rtimer_timeout(struct rtimer *timer, void *ptr)
{
  /* Re-arm rtimer. Starting up the sensor takes around 125ms */
  /* rtimer period 2s */
  // clock_time_t t;
  // t = clock_time();
  rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0, do_rtimer_timeout, NULL);

  // int s, ms1,ms2,ms3;
  // s = clock_time() / CLOCK_SECOND;
  // ms1 = (clock_time()% CLOCK_SECOND)*10/CLOCK_SECOND;
  // ms2 = ((clock_time()% CLOCK_SECOND)*100/CLOCK_SECOND)%10;
  // ms3 = ((clock_time()% CLOCK_SECOND)*1000/CLOCK_SECOND)%10;
  
  // counter_rtimer++;
  // printf(": %d (cnt) %ld (ticks) %d.%d%d%d (sec) \n",counter_rtimer,t, s, ms1,ms2,ms3); 
  // get_light_reading();

  if (check_change_in_mpu() || check_change_in_lux()) {
      int i = 4;
      
      while (i > 0) {
        buzzer_start(2093);
        clock_wait(CLOCK_SECOND * 2);
        buzzer_stop();
        clock_wait(CLOCK_SECOND * 2);

        i--;
      }
    }
}

static int
get_light_reading()
{
  int value;
  init_opt_reading();

  value = opt_3001_sensor.value(0);
  if(value != CC26XX_SENSOR_READING_ERROR) {
    printf("OPT: Light=%d.%02d lux\n", value / 100, value % 100);
    return value;
  } else {
    printf("OPT: Light Sensor's Warming Up\n\n");
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

static int
check_change_in_mpu()
{
  int significant_change = 400;

  int new_mpu = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_X);
  if (abs(new_mpu - curr_mpu) > significant_change) {
    printf("The mpu has changed from %d to %d\n", curr_mpu, new_mpu);
    curr_mpu = new_mpu;
    return 1;
  } else {
    // printf("The mpu has not changed\n");
    curr_mpu = new_mpu;
    return 0;
  }
}

static void
init_opt_reading(void)
{
  SENSORS_ACTIVATE(opt_3001_sensor);
}

PROCESS_THREAD(process_rtimer, ev, data)
{
  PROCESS_BEGIN();
  init_opt_reading();

  // printf(" The value of RTIMER_SECOND is %d \n",RTIMER_SECOND);
  // printf(" The value of timeout_rtimer is %ld \n",timeout_rtimer);

  while(1) {
    
    // // Buzz for 2 seconds if changes detected
    // if (check_change_in_lux() || check_change_in_mpu()) {
    //   int i = 4;
      
    //   while (i > 0) {
    //     buzzer_start(2093);
    //     clock_wait(CLOCK_SECOND * 2);
    //     buzzer_stop();
    //     clock_wait(CLOCK_SECOND * 2);

    //     i--;
    //   }
    // }

    rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0,  do_rtimer_timeout, NULL);

    PROCESS_YIELD();
  }

  PROCESS_END();
}
