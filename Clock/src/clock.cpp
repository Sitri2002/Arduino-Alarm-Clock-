#include <avr/interrupt.h>
#include "global_header.h"
#include "clock.h"
#include "PWM.h"

// Current system state (initially idle state) (global variable in main)
extern volatile stateType state;

// Current clock time digits and AM/PM status (global variables in main) 
extern volatile char time_digits [TIME_DIGITS_NUMBER];
extern volatile char am_pm; // 0 is AM and 1 is PM
extern volatile int hour_mode;
// Alarm time digits and AM/PM status (global variables in main)
extern volatile char alarm_time_digits [TIME_DIGITS_NUMBER];
extern volatile char alarm_am_pm;

// Buffer time digits and AM/PM status (global variables in main) used to set times
extern volatile char buffer_time_digits [TIME_DIGITS_NUMBER];
extern volatile char buffer_am_pm;

// Referemce time digits and AM/PM status used to check if the buffer changed
volatile char reference_time_digits [TIME_DIGITS_NUMBER];
volatile char reference_am_pm;

// Status of alarm being activated or deactivated (global variable in main)
extern volatile char alarm_activation; // 1 is activated, 0 is deactivated

// Counter that gets incremented every 4 seconds to signal a full minute has 
// passed when it reaches 15.
volatile char counter = 0;

// Initializes clock timer (timer 1) which triggers the clock timer interrupt 
// every 4 seconds.
void init_clock(){

    // Setting timer 1 into CTC mode
    TCCR1B |= ( 1 << WGM12); 

    // Setting compare A register to 62499 = (4*16000000/1024) - 1 to count for 4 seconds 
    // given a prescaler of 1024. CLOCK_SPEED_FACTOR is a debug constant to accelerate the 
    // clock. In production, it is set to 1.
    OCR1A = (int)(62499/CLOCK_SPEED_FACTOR);

    // Sets timer 1 prescaler to 1024
    TCCR1B  |= (1 << CS12) | (1 << CS10);
    
    // Reset clock timer counter to 0 before starting it
    TCNT1 = 0;
    
    // Enables interrupt on timer 1 compare A
    TIMSK1 |= (1 << OCIE1A);

}

// Loads time given in input_time_digits and input_am_pm to the buffer. 
// Disables then reenables clock timer during read. Also loads time into reference.
void load_time_to_buffer(char * input_time_digits, char input_am_pm){

    // Disables clock timer
    TIMSK1 &= ~(1 << OCIE1A);

    buffer_time_digits[0] = input_time_digits[0];
    buffer_time_digits[1] = input_time_digits[1];
    buffer_time_digits[2] = input_time_digits[2];
    buffer_time_digits[3] = input_time_digits[3];
    buffer_am_pm = input_am_pm;

    reference_time_digits[0] = buffer_time_digits[0];
    reference_time_digits[1] = buffer_time_digits[1];
    reference_time_digits[2] = buffer_time_digits[2];
    reference_time_digits[3] = buffer_time_digits[3];
    reference_am_pm = buffer_am_pm;

    // Enables clock timer
    TIMSK1 |=  (1 << OCIE1A);
}

// Saves from buffer to the time digits and AM/PM status given in output_time_digits 
// and output_am_pm. Only save time if it is valid and differs from its original value. 
// Disables then reenables clock timer during write. Also resets clock timer counter 
// if the clock time has been changed.
void save_buffer_to_time(char * output_time_digits, char * output_am_pm){

    // Disables clock timer
    TIMSK1 &= ~(1 << OCIE1A);

    if(buffer_has_valid_time() && buffer_differs_from_refernce()){
        // if hours is set in range (13->23), automatically switch to 24hr mode
        output_time_digits[0] = buffer_time_digits[0];
        output_time_digits[1] = buffer_time_digits[1];
        output_time_digits[2] = buffer_time_digits[2];
        output_time_digits[3] = buffer_time_digits[3];
        *output_am_pm = buffer_am_pm;
        
        // Reset clock timer counter to 0 before starting it if clock time was changed
        if(output_am_pm == &am_pm){
            TCNT1 = 0;
        }
    }

    // Enables clock timer
    TIMSK1 |=  (1 << OCIE1A);
}

// checks if time buffer has valid time
int buffer_has_valid_time(){

    int hours = buffer_time_digits[0]*10 + buffer_time_digits[1];
    int minutes = buffer_time_digits[2]*10 + buffer_time_digits[3];
    // valid time means hours are between 1 and 12 inclusive and minutes are less than 60 for 12hr mode
    if (hour_mode == 0) {
        if(hours <= 12 && hours >= 1 && minutes < 60)
            return 1;
        else
            return 0;
    }
    else {
        // for 24hr mode, hours is between 0->23 and am_pm not active
        if(hours <= 23 && hours >= 0 && minutes < 60 && buffer_am_pm == 0)
            return 1;
        else
            return 0;
    }

}

// checks if buffer differs from its original value
int buffer_differs_from_refernce(){

    if(buffer_time_digits[0] == reference_time_digits[0] &&
        buffer_time_digits[1] == reference_time_digits[1] &&
        buffer_time_digits[2] == reference_time_digits[2] &&
        buffer_time_digits[3] == reference_time_digits[3] &&
        buffer_am_pm == reference_am_pm)
        return 0;
    else
        return 1;
    
}

void switch_12_24(char* digits, char* var_am_pm) {
    int temp_hour;
    if (hour_mode == 0) {
        //change current clock to 24hr mode
        if (digits[0]*10 + digits[1] == 12) {
            digits[0] = 0;
            digits[1] = 0;
        } 
        if ((*var_am_pm == 1)) {
            // time is changed when time = 13:00+
            temp_hour = digits[0]* 10 + digits[1] + 12;
            digits[0] = temp_hour/10;
            digits[1] = temp_hour%10;
        }
        *var_am_pm = 0;
    }
    else {
        //change current clock to 12 hr mode
        //for 13:00+ cases
        if (digits[0]*10 + digits[1]> 12) {
            temp_hour = digits[0]* 10 + digits[1] - 12;
            digits[0] = temp_hour/10;
            digits[1] = temp_hour%10;
            *var_am_pm = 1;
        }
        if (digits[0] == 1 && digits[1] == 2) {
            *var_am_pm = 1;
        }
        if (digits[0] == 0 && digits[1] == 0) {
            digits[0] = 1;
            digits[1] = 2;
        }
    }
        //for 12:00 -> 12:59 cases, only turn to pm
}

//change current time to its corresponding hour mode
void change_hour_mode () {
    switch_12_24((char*)time_digits, (char*) (&am_pm));
    switch_12_24((char*)alarm_time_digits, (char*) (&alarm_am_pm));
    switch_12_24((char*)buffer_time_digits, (char*) (&buffer_am_pm));
    switch_12_24((char*)reference_time_digits, (char*) (&reference_am_pm));
    hour_mode = !hour_mode;
}




// Clock timer routine executed every 4 seconds
ISR(TIMER1_COMPA_vect){

    // increment counter
    counter += 1;

    // If counter has reached 15, 1 minute has passed, so counter gets reset, otherwise 
    // return
    if(counter < 15)
        return;
    
    // reset counter
    counter = 0;

    // increment last digit of minutes
    time_digits[3] += 1;

    // If last digit of minutes is 10, it resets to 0 and the first digit of minutes 
    // is incremented 
    if(time_digits[3] > 9){
        time_digits[3] = 0;
        time_digits[2] += 1;
    }

    // If first digit of minutes is 6, it resets to 0 and the last digit of hours 
    // is incremented 
    if(time_digits[2] > 5){
        time_digits[2] = 0;
        time_digits[1] += 1;
    } 

    // If last digit of hours is 10, it resets to 0 and the first digit of hours 
    // is incremented 
    if(time_digits[1] > 9){
        time_digits[1] = 0;
        time_digits[0] += 1;
    } 

    if (hour_mode == 0) {
    // If hours reach 13, they are reset to 1 for 12hr mode
        if(time_digits[0]==1 && time_digits[1]==3){
            time_digits[0] = 0;
            time_digits[1] = 1;
        }
    }
    else {
        // If hours reach 24, they are reset to 0 for 24hr mode
        if(time_digits[0]==2 && time_digits[1]==4){
            time_digits[0] = 0;
            time_digits[1] = 0;
        }
    }
    // Toggle AM/PM time when clock reaches 12:00
    if (hour_mode == 0 ) {
        if(time_digits[0] == 1 && time_digits[1] == 2 && time_digits[2] == 0 && time_digits[3] == 0)
            am_pm = !am_pm;
    }

    // Trigger alarm if clock time and alarm time are the same.
    if((state == show_time) && alarm_activation){
        if(time_digits[0] == alarm_time_digits[0] &&
            time_digits[1] == alarm_time_digits[1] &&
            time_digits[2] == alarm_time_digits[2] &&
            time_digits[3] == alarm_time_digits[3] &&
            am_pm == alarm_am_pm){
                turn_on_alarm();
                state = alarm_on;
            }
    }
} 
    


