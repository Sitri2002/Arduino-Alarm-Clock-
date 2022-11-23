#include <avr/io.h>
#include "ADC.h"


// Initialize the ADC converter to use a reference voltage of VCC 
// (5V) with a right-adjusted result in the ADC(H/L) registers using 
// pin A15 as input (its digital functionality is disabled) with an ADC 
// clock prescaler of 2 and no gain. The ADC converter is enabled at the 
// end of this configuration.
void initADC(){

    // Set reference voltage to VCC (5V)
    ADMUX |=  (1 << REFS0);
    ADMUX &= ~(1 << REFS1);

    // Right adjust ADC result in ADC(H/L) registers
    ADMUX &= ~(1 << ADLAR);

    // Set ADC to use ADC15 as single-ended input with no gain
    ADMUX |= (1 << MUX0) | (1 << MUX1) | (1 << MUX2);
    ADCSRB |= (1 << MUX5);

    // Use minimal prescaler for ADC clock (2)
    ADCSRA &= ~((1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2));

    // Turn off digital input functionality of A15
    DIDR2 |=  (1 << ADC15D);

    // Enable the ADC
    ADCSRA |=  (1 << ADEN);

}

// Starts an ADC conversion on pin A15 and returns the result once 
// the conversion is done.
int digital_read_A15(){

    // Start ADC conversion process
    ADCSRA |=  (1 << ADSC);

    // Wait till ADC conversion process ends
    while(ADCSRA & (1 << ADSC));

    // Read result of ADC conversion process
    int a = ADCL;
    int b = ADCH;

    // concatenate the lower 2 bits of ADCH with the bits of ADCL
    // to form the 10 bit result
    int c = ((b % 4) << 8) + a;

    return c;

}