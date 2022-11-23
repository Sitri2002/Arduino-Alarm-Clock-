#include <avr/io.h>
#include "switch.h"

// Sets pin PD0 as an input pin and enables its pullup resistor for stable input.
// It also enables its interrupt (INT0) with any-edge trigger
void initSwitchPD0(){

    // set PD0 direction for input
    DDRD &= ~(1 << DDD0);

    // enable the PD0 pullup resistor for stable input
    PORTD |= (1 << PORTD0);

    // enable the interrupt for PD0
    EIMSK |= (1 << INT0);

    // set the interrupt to trigger on any edge of the input signal
    EICRA |=  (1 << ISC00);
    EICRA &= ~(1 << ISC01);
}

// enables the PD0 pin switch interrupt
void enable_switch_interrupt(){

    // enable the interrupt for PD0
    EIMSK |=  (1 << INT0);

}

// disables the PD0 pin switch interrupt
void disable_switch_interrupt(){

    // disables the interrupt for PD0
    EIMSK &= ~(1 << INT0);

}
