#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/interrupt.h>
#include "global_header.h"
#include "display.h"
#include "clock.h"
#include "remote.h"
#include "PWM.h"
#include "I2C.h"
#include "Arduino.h"

// Current system state (initially idle state) (global variable in main)
volatile stateType state = show_time;

// Current clock time digits and AM/PM status (global variables in main) 
volatile char time_digits [TIME_DIGITS_NUMBER]= INITIAL_TIME;
volatile char am_pm = INITIAL_AM_PM; // 0 is AM and 1 is PM
volatile int hour_mode = 0; //0 is 12hr mode and 1 is 24hr mode
// Alarm time digits and AM/PM status (global variables in main)
volatile char alarm_time_digits [TIME_DIGITS_NUMBER]= INITIAL_ALARM;
volatile char alarm_am_pm = INITIAL_ALARM_AM_PM;

// Buffer time digits and AM/PM status (global variables in main) used to set times
volatile char buffer_time_digits [TIME_DIGITS_NUMBER];
volatile char buffer_am_pm;

// Current digit selected in set_time and set_alarm states (global variables in main)
volatile char cursor_digit = 0;

// Status of alarm being activated or deactivated (global variable in main)
volatile char alarm_activation = 1; // 1 is activated, 0 is deactivated


int main() {

    // Initializes clock timer (timer 0) which triggers the clock timer interrupt 
    // every 4 seconds.
    init_clock();

    // Initialize display system including 4-digit 7-segment display, AM/PM 
    // LED and alarm LED along with their display refresh timer.
    init_display();

    // Initialize remote with its timer (timer 3) for receiving IR NEC signals.
    init_remote();

    // Initialize alarm with its timers (timer 4 and timer 5)
    initPWM();

    // Initialize the I2C module
    InitI2C();

    // Initialize the MPU by waking it up
    InitMPU();

    // enable global interrupts (multiple interrupts are used in the program)
    // interrupts are enabled after initialization procedure
    sei();

    // button click received from remote
    char button;
    
    // Infinite interaction loop which handles inputs from user through remote
    while (1) {

        // Check if the MPU is detecting movement
        if(check_movement()){
            if(state == alarm_on){
                turn_off_alarm();
                state = show_time;
            }
        }

        // get inputs from remote
        button = get_remote_input();

        if(button == 0)
            continue;

        // button to activated/deactivate alarm trigger
        if(button == 'A'){
            if(state == show_time)
                alarm_activation = !alarm_activation;
        }

        // S is the button to turn off the alarm
        if(button == 'S'){
            if(state == alarm_on){
                turn_off_alarm();
                state = show_time;
            }
        }

        // M is the button to change mode between show_time, set_time, and set_alarm. 
        // It also resets the cursor to the first digit and loads/saves buffer time 
        // to set clock time and alarm time.
        if(button == 'M'){
            if(state == show_time){
                // set buffer time to clock time
                load_time_to_buffer((char *)time_digits, am_pm);
                state = set_time;
            }
            else if(state == set_time){
                // save buffer time to clock time
                save_buffer_to_time((char *)time_digits, (char *)(&am_pm));
                // set buffer time to alarm time
                load_time_to_buffer((char *)alarm_time_digits, alarm_am_pm);
                state = set_alarm;
            }
            else if(state == set_alarm){
                // save buffer time to alarm time
                save_buffer_to_time((char *)alarm_time_digits, (char *)(&alarm_am_pm));
                state = show_time;
            }
            cursor_digit = 0;
        }
        // if Vol- button is pressed, change the current time to the correct mode if needed and change hour_mode id
        if (button == 'P') {
            change_hour_mode();
        }

        // These are the controls to set time and set alarm
        if(state == set_time || state == set_alarm){

            // 0 to 9 are the buttons to change the buffer digit at the cursor. 
            // Also create a right button click to move cursor.
            if(button >= '0' && button <= '9'){
                buffer_time_digits[(int)cursor_digit] = button - '0';
                button = 'R';
            }
            // / is the button to switch between am and pm for the buffer, if in 12hr mode
            else if(button == '/'){
                if (hour_mode == 0) {
                    buffer_am_pm = !buffer_am_pm;
                }
            }

            // R and L are the buttons to move the cursor right and left with 
            // wrapping up at the ends.
            if(button == 'R'){
                cursor_digit += 1;
                if(cursor_digit == 4)
                    cursor_digit = 0;
            }
            else if(button == 'L'){
                cursor_digit -= 1;
                if(cursor_digit == -1)
                    cursor_digit = 3;
            }
        }
    }
    return 0;

}