#ifndef PWM_H
#define PWM_H 

// Initialize (PH5) to be a Fast non-inverting mode PWM 
// output for timer 4 (OC4C) with a variable TOP (OC4RA) with a prescaler 
// of 1 and a duty cycle of 50%. The frequency is set to 15 kHz by default 
// with the alarm turned off. Also turns on timer 5 as the timer which varies
// the frequency of the alarm to create a chirping noise.
void initPWM();

// Set PWM frequency by changing top of PWM counter (prescaler is 1) based on 
// frequency given thorugh frequency parameter. Duty cycle is maintained at 50% by 
// setting PWM toggle counter value to half of TOP (values are rounded).
void SetPWMfrequency(unsigned int frequency);

// turns off alarm by turning off output pin and deactivating timers
void turn_off_alarm();

// turns on alarm by turning on output pin and activating timers
void turn_on_alarm();

#endif