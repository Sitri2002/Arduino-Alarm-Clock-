#ifndef SWITCH_H
#define SWITCH_H

// Sets pin PD0 as an input pin and enables its pullup resistor for stable input.
// It also enables its interrupt (INT0) with any-edge trigger
void initSwitchPD0();

// enables the PD0 pin switch interrupt
void enable_switch_interrupt();

// disables the PD0 pin switch interrupt
void disable_switch_interrupt();

#endif
