#include <avr/io.h>
#include <avr/interrupt.h>
#include "global_header.h"
#include "display.h"

// Current system state (initially idle state) (global variable in main)
extern volatile stateType state;
extern volatile int hour_mode;
// Current clock time digits and AM/PM status (global variables in main) 
extern volatile char time_digits [TIME_DIGITS_NUMBER];
extern volatile char am_pm; // 0 is AM and 1 is PM

// Alarm time digits and AM/PM status (global variables in main)
extern volatile char alarm_time_digits [TIME_DIGITS_NUMBER];
extern volatile char alarm_am_pm;

// Buffer time digits and AM/PM status (global variables in main) used to set times
extern volatile char buffer_time_digits [TIME_DIGITS_NUMBER];
extern volatile char buffer_am_pm;

// Status of alarm being activated or deactivated (global variable in main)
extern volatile char alarm_activation; // 1 is activated, 0 is deactivated

// Current digit from time_digits to be displayed on the 4-digit 7-segment display
volatile char time_digit_select = 0;
// Current digit selected in set_time and set_alarm states (global variables in main)
extern volatile char cursor_digit;
// Cursor blink state
volatile char cursor_blink = 0;
// Count to give delay for cusor blink
volatile char cursor_count = 0;

// Seven segment display patterns with bit encoding to display segments: 7:0 => DP,G,F,E,D,C,B,A
unsigned char patterns[11] = {
    0x3F, // 0:    0b00111111
    0x06, // 1:    0b00000110
    0x5B, // 2:    0b01011011
    0x4F, // 3:    0b01001111
    0x66, // 4:    0b01100110
    0x6D, // 5:    0b01101101
    0x7D, // 6:    0b01111101
    0x07, // 7:    0b00000111
    0x7F, // 8:    0b01111111
    0x6F, // 9:    0b01101111
    0x00  // None: 0b00000000 (DP light only)
};

// Initialize display system including 4-digit 7-segment display, AM/PM 
// LED and alarm LED. Also initializes display timer (timer 0) which triggers 
// the display timer interrupt about 4*60 times per second. The 4*60 Hz rate for
// the display timer is to obtain 60 Hz display rate considering each digit has to
// be displayed in its own interrupt routine for a proper sweep.
void init_display() {

    // Sets PORTF[4:2], PORTL[1,3,5,7], and PORTG1 to be segment outputs to display a digit
    //DDRF |= (1<< DDF4)|(1<< DDF2) | (1<< DDF3);
    //DDRL |= (1<< DDL7)|(1<< DDL3)|(1<< DDL1)|(1<< DDL5);
    //DDRG |= (1<< DDG1);

    DDRA = 0xFF;

    // Also sets PH6 to be the AM/PM LED output and PB4 to be the alarm output.
    //DDRC = (1 << DDC4) | (1 << DDC5);
    DDRB |= (1 << DDB4);
    DDRH |= (1 << DDH6);
    DDRB |= (1<<DDB0);
    // PC4 permanent ground
    //DDRC |= (1 << DDC4);
    // PC1 is right-led input (AM/PM)
    //DDRC |= (1 << DDC1);
    // PK7 is left-led input (alarm)
    //DDRK |= (1 << DDK7);

    // Set timer 0 prescaler to 256
    TCCR0B  |= (1 << CS02);
    
    // Enable timer 0 overflower interrupt
    TIMSK0 |= (1 << TOIE0);

}

// Displays a number (digit) given in time_digit_value at the digit location 
// given in time_digit on the 4-digit 7-segment display. PORTF[7:5] and PORTK4 are used to select 
// which digit position to display the number at. Setting one of these pins to output 
// makes it a ground which activates the digit position while setting it to input makes it 
// high impedance which deactivates the digit position. The digit pins are used to send 
// the digit bit encoding.
void display_time_digit(unsigned char time_digit, unsigned char time_digit_value){

    // Set PORTA to output the encoding of number time_digit_value
    unsigned char pattern = patterns[time_digit_value];

    // if a bit in the pattern is 1, set its corresponding pin to 1, otherwise, set it to 0
    // set_bit_to_pin(pattern, 0, PORTF, PORTF4);
    // set_bit_to_pin(pattern, 1, PORTF, PORTF2);
    // set_bit_to_pin(pattern, 2, PORTL, PORTL7);
    // set_bit_to_pin(pattern, 3, PORTL, PORTL3);
    // set_bit_to_pin(pattern, 4, PORTL, PORTL1);
    // set_bit_to_pin(pattern, 5, PORTF, PORTF3);
    // set_bit_to_pin(pattern, 6, PORTG, PORTG1);
    // set_bit_to_pin(pattern, 7, PORTL, PORTL5);

    PORTA = patterns[time_digit_value];
    DDRC = (DDRC & 0xF0) | (1 << time_digit);
    if (hour_mode) {
        PORTB |= (1<<PORTB0);
    }
    else {
        PORTB &= ~(1<<PORTB0);
    }
    
    // Enable digit position time_digit by setting its pin to output (ground) and others
    // to input (high impedance)
    // DDRF &= ~((1 << DDF7)|(1 << DDF6)|(1 << DDF5));
    // DDRK &= ~(1 << DDK4);
    // switch(time_digit){
    //     case 0:
    //         DDRF |= (1 << DDF7);
    //         break;
    //     case 1:
    //         DDRF |= (1 << DDF6);
    //         break;
    //     case 2:
    //         DDRF |= (1 << DDF5);
    //         break;
    //     case 3:
    //         DDRK |= (1 << DDK4);
    //         break;
    // }
}

// Display timer interrupt that triggers at 4*60 Hz rate. Displays one of the 4 digits on 
// the 4-digit 7-segment display, then changes which digit to display for next time. 
// Also displays the AM/PM LED status.
ISR(TIMER0_OVF_vect){

    // Displays one of the 4 digits on the 4-digit 7-segment display and AM/PM status from buffer
    if(state == set_time || state == set_alarm){
        // count up to change blink state or selected digit
        cursor_count += 1;
        if(cursor_count == 60){
            cursor_count = 0;
            cursor_blink = !cursor_blink;
        }
        // selected digit is in blink mode, show empty digit instead (10)
        if(time_digit_select == cursor_digit && cursor_blink){
            display_time_digit(time_digit_select, 10);
        }
        else{
            display_time_digit(time_digit_select, buffer_time_digits[(int)time_digit_select]);
        }

        // light up the AM/PM LED if buffer am_pm is true (1)
        // if(buffer_am_pm)
        //     PORTC |=  (1 << PORTC1);
        // else
        //     PORTC &= ~(1 << PORTC1);
        if(buffer_am_pm)
            PORTH |=  (1 << PORTH6);
        else
            PORTH &= ~(1 << PORTH6);
    }
    // Displays one of the 4 digits on the 4-digit 7-segment display from clock and clock 
    // AM/PM
    else{
        display_time_digit(time_digit_select, time_digits[(int)time_digit_select]);
        
        // light up the AM/PM LED if am_pm is true (1)
        // if(am_pm)
        //     PORTC |=  (1 << PORTC1);
        // else
        //     PORTC &= ~(1 << PORTC1);
        if(am_pm)
            PORTH |=  (1 << PORTH6);
        else
            PORTH &= ~(1 << PORTH6);
    }

    // changes which digit to display for next time
    time_digit_select += 1;
    if(time_digit_select == TIME_DIGITS_NUMBER) time_digit_select = 0;

    // switch(state){
    //     case show_time:
    //         if(alarm_activation)
    //             PORTK |= (1 << PORTK7);
    //         else
    //             PORTK &= ~(1 << PORTK7);
    //         break;
    //     case set_time:
    //         PORTK &= ~(1 << PORTK7);
    //         break;
    //     case set_alarm:
    //         PORTK |= (1 << PORTK7);
    //         break;
    //     case alarm_on:
    //         if(alarm_activation)
    //             PORTK |= (1 << PORTK7);
    //         else
    //             PORTK &= ~(1 << PORTK7);
    //         break;
    // }

    

    switch(state){
        case show_time:
            if(alarm_activation)
                PORTB |= (1 << PORTB4);
            else
                PORTB &= ~(1 << PORTB4);
            break;
        case set_time:
            PORTB &= ~(1 << PORTB4);
            break;
        case set_alarm:
            PORTB |= (1 << PORTB4);
            break;
        case alarm_on:
            if(alarm_activation)
                PORTB |= (1 << PORTB4);
            else
                PORTB &= ~(1 << PORTB4);
            break;
    }

}