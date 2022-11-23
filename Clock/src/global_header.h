#ifndef GLOBAL_H
#define GLOBAL_H 

/*Debug parameters*/ 

// CLOCK_SPEED_FACTOR is a debug constant to accelerate the clock. 
// In production, it is set to 1.
#define CLOCK_SPEED_FACTOR 300

/*Parameters*/ 

// Initial value for time
#define INITIAL_TIME {1,2,0,0}

// Initial value for AM/PM
#define INITIAL_AM_PM 0

// Initial value for alarm
#define INITIAL_ALARM {1,2,1,0}

// Initial value for alarm AM/PM
#define INITIAL_ALARM_AM_PM 0

/*Constants*/ 
#define TIME_DIGITS_NUMBER 4 

/* State machine enums*/

// State machine to control general behavior of system. Idle state is show_time.
// Alarm on is when alarm is triggered. set_time and set_alarm are for setting the time 
// and alarm time.
typedef enum stateType_enum {
    show_time, alarm_on, set_time, set_alarm}
stateType;


/*macro functions*/

#define set_bit_to_pin(number, bit, port, pin) \
if((number >> bit) % 2) port |= (1 << pin); else port &= ~(1 << pin);

#endif

