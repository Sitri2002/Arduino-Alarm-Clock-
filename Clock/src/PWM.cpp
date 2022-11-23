#include <avr/io.h>
#include <avr/interrupt.h>
#include "PWM.h"

#define CLKFREQ 16000000
#define DEFAULT_FREQUENCY 15000

// current alarm frequency
int alarm_freq = 1000;

// Initialize (PH5) to be a Fast non-inverting mode PWM 
// output for timer 4 (OC4C) with a variable TOP (OC4RA) with a prescaler 
// of 1 and a duty cycle of 50%. The frequency is set to 15 kHz by default 
// with the alarm turned off. Also turns on timer 5 as the timer which varies
// the frequency of the alarm to create a chirping noise.
void initPWM(){

    // Set PH5 be an output pin
    DDRH |= (1 << DDH5);

    // Set Timer 4 to Fast PWM 10-bit mode, non-inverting with prescaler of 1
    TCCR4A |= (1 << COM4C1)|(1 << WGM41)|(1 << WGM40);
    TCCR4B |= (1 << WGM42)|(1 << CS40) | (1 << WGM43);

    // turn off alarm by default
    turn_off_alarm();

    // default frequency of 15 kHz with 50% duty cycle
    SetPWMfrequency(DEFAULT_FREQUENCY);

    // Set timer 5 prescaler to 1 in normal mode
    TCCR5B |= (1 << CS50);

    // Enable timer 5 comp A interrupt
    TIMSK5 |= (1 << OCIE5A);

    // Set timer 5 compare value to this custom value for chiping noise
    OCR5A = 5000;

}

// Set PWM frequency by changing top of PWM counter (prescaler is 1) based on 
// frequency given thorugh frequency parameter. Duty cycle is maintained at 50% by 
// setting PWM toggle counter value to half of TOP (values are rounded).
void SetPWMfrequency(unsigned int frequency){
    OCR4A = (int) (CLKFREQ / frequency) - 1;
    OCR4C = (int) (OCR4A/2);
}


// turns off alarm by turning off output pin and deactivating timers
void turn_off_alarm(){

    DDRH &= ~(1 << DDH5);
    TCCR4A &= ~(1 << COM4C1);
    TCCR4B &= ~(1 << CS40);
    TCCR5B &= ~(1 << CS50);

}

// turns on alarm by turning on output pin and activating timers
void turn_on_alarm(){

    DDRH |= (1 << DDH5);
    TCCR4A |= (1 << COM4C1);
    TCCR4B |= (1 << CS40);
    TCCR5B |= (1 << CS50);

}

// Interrupt routine for timer 5 which handles changing the frequency of the alarm 
// to create a chirping noise.
ISR(TIMER5_COMPA_vect){
    alarm_freq += 50;
    SetPWMfrequency(alarm_freq);
    if(alarm_freq == 4000)
        alarm_freq = 1000;
}