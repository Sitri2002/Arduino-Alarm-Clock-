#ifndef DISPLAY_H
#define DISPLAY_H 

// Initialize display system including 4-digit 7-segment display, AM/PM 
// LED and alarm LED. Also initializes display timer (timer 0) which triggers 
// the display timer interrupt about 4*60 times per second. The 4*60 Hz rate for
// the display timer is to obtain 60 Hz display rate considering each digit has to
// be displayed in its own interrupt routine for a proper sweep.
void init_display();

// Displays a number (digit) given in time_digit_value at the digit location 
// given in time_digit on the 4-digit 7-segment display. PORTC[3:0] is used to select 
// which digit position to display the number at. Setting one of these pins to output 
// makes it a ground which activates the digit position while setting it to input makes it 
// high impedance which deactivates the digit position.
void display_time_digit(unsigned char time_digit, unsigned char time_digit_value);

#endif
