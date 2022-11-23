#ifndef ADC_H
#define ADC_H 

// Initialize the ADC converter to use a reference voltage of VCC 
// (5V) with a right-adjusted result in the ADC(H/L) registers using 
// pin A15 as input (its digital functionality is disabled) with an ADC 
// clock prescaler of 2 and no gain. The ADC converter is enabled at the 
// end of this configuration.
void initADC();

// Starts an ADC conversion on pin A15 and returns the result once 
// the conversion is done.
int digital_read_A15();

#endif