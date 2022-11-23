#include <avr/io.h>
// #include <Arduino.h>
#include "I2C.h"
#include <Arduino.h>

#define SLA 0x68 // MPU address when AD0 grounded
#define PWR_MGMT 0x6B // Power management register address
#define WAKEUP 0x00 // PWR_MGMT value to wakeup MPU to normal operation mode
#define SL_MEMA_XAX_HIGH 0x3B // register address for high nibble of X-axis acceleration sensor data 
#define SL_MEMA_XAX_LOW 0x3C // register address for low nibble of X-axis acceleration sensor data 
#define SL_MEMA_YAX_HIGH 0x3D // register address for high nibble of Y-axis acceleration sensor data 
#define SL_MEMA_YAX_LOW 0x3E // register address for low nibble of Y-axis acceleration sensor data 
#define SL_MEMA_ZAX_HIGH 0x3F // register address for high nibble of Z-axis acceleration sensor data 
#define SL_MEMA_ZAX_LOW 0x40 // register address for low nibble of Z-axis acceleration sensor data 


// Initializes the I2C module by waking it up and setting bitrate to 10kHz
void InitI2C(){

    // Wake up I2C module on mega 2560
    PRR0 &= ~(1 << PRTWI);
    
    // Set prescaler power TWPS to 1
    TWSR  |=  (1 << TWPS0);
    TWSR  &= ~(1 << TWPS1);

    // Set bitrate to 10 kHz: TWBR = (16 MHz/10kHz - 16) / (2 * 4 ^ 1)
    TWBR = 198;
    
    // Enable I2C module
    TWCR = (1 << TWINT) | (1 << TWEN);
    TCCR2A &= ~(1 << WGM10);
    TCCR2A &= ~(1 << WGM11);
    TCCR2B |= (1 << WGM12);
    TCCR2B &= ~(1 << WGM13);

}


/* This delays the program an amount of microseconds specified by unsigned int delay.
*/
void delayUs(unsigned int delay){
    // Use prescaler 1, OCR1A = 15
    unsigned int count = 0;
    TCCR2B |= (1 << CS10);
    TCCR2B &= ~(1 << CS11);
    TCCR2B &= ~(1 << CS12);
    OCR2A = 15;
    //OCR1AH = 0x00;
    //OCR1AL = 0x02;
    TIFR2 |= (1 << OCF2A);
    TCNT2 = 0;
    
    while (count < delay) {
        if (TIFR2 & (1<<OCF2A)) {
            count++;
            TIFR2 |= (1 << OCF2A);
        }  
    }
    // turn off timer
    TCCR2B &= ~ ((1<<CS12) | (1<<CS10) | (1<<CS11));
}
// Start I2C communication with a start condition and SLA + W address frame
void StartI2C_Trans(unsigned char sla){

    // trigger a start
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTA);

    // wait for completition
    while(!(TWCR & (1 << TWINT)));

    // write data SLA + W
    TWDR = (sla << 1) + 0x00;
    
    // trigger
    TWCR = (1 << TWINT) | (1 << TWEN);

    // wait for completition
    while(!(TWCR & (1 << TWINT)));

}

// stop I2C communications with a stop condition
void StopI2C_Trans(){
    // trigger stop
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
}

// write 8-bit data given in parameter data to the I2C data register 
// to be sent to the I2C bus
void write(unsigned char data){

    // Write data in register to be sent to I2C
    TWDR = data;
    
    // trigger
    TWCR = (1 << TWINT) | (1 << TWEN);

    // wait for completition
    while(!(TWCR & (1 << TWINT)));

}


// Read data from slave with address SLA at memory register address MEMADDRESS 
// through the I2C bus using the read sub-protocol
void Read_from(unsigned char sla, unsigned char MEMADDRESS){

    // Start a transmission to the SLA
    StartI2C_Trans(sla);

    // write the register being read
    write(MEMADDRESS);

    // trigger a repeated start
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTA);

    // wait for completition
    while(!(TWCR & (1 << TWINT)));

    // SLA + R
    TWDR = (sla << 1) + 0x01;
    
    // trigger
    TWCR = (1 << TWINT) | (1 << TWEN);

    // wait for completition
    while(!(TWCR & (1 << TWINT)));
    
    // trigger
    TWCR = (1 << TWINT) | (1 << TWEN);

    // wait for completition
    while(!(TWCR & (1 << TWINT)));

    // trigger a stop
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);

}

// Load 8-bit data from the I2C data register
unsigned char Read_data(){
    return TWDR;
}


// Wake up MPU chip by writing WAKEUP value to PWR_MGMT register address
void InitMPU(){
    StartI2C_Trans(SLA);
    write(PWR_MGMT);
    write(WAKEUP);
    StopI2C_Trans();

    // Initialize USB communication to print sensor readings to computer console
    //  Serial.begin(9600);
}

// Uses the MPU readings to check for movement
int check_movement() {

    int x_reading, y_reading, z_reading;
    
    // Read MSb half the of the accelerometer value for the x axis
    Read_from(SLA, SL_MEMA_XAX_HIGH);
    // Load the data into x_reading
    x_reading = Read_data();
    // Read LSb half the of the accelerometer value for the x axis
    Read_from(SLA, SL_MEMA_XAX_LOW);
    // Load and concatenate the LSb data with the MSb data into x_reading
    x_reading = (x_reading << 8) + Read_data();

    // Read MSb half the of the accelerometer value for the y axis
    Read_from(SLA, SL_MEMA_YAX_HIGH);
    // Load the data into y_reading
    y_reading = Read_data();
    // Read LSb half the of the accelerometer value for the y axis
    Read_from(SLA, SL_MEMA_YAX_LOW);
    // Load and concatenate the LSb data with the MSb data into y_reading
    y_reading = (y_reading << 8) + Read_data();

    // Read MSb half the of the accelerometer value for the z axis
    Read_from(SLA, SL_MEMA_ZAX_HIGH);
    // Load the data into z_reading
    z_reading = Read_data();
    // Read LSb half the of the accelerometer value for the z axis
    Read_from(SLA, SL_MEMA_ZAX_LOW);
    // Load and concatenate the LSb data with the MSb data into z_reading
    z_reading = (z_reading << 8) + Read_data();

    // stop the I2C transmission
    StopI2C_Trans();

    //Print all three readings to the computer console though the USB
    // Serial.print("X_READING = ");
    // Serial.println(x_reading);
    // Serial.print("Y_READING = ");
    // Serial.println(y_reading);
    // Serial.print("Z_READING = ");
    // Serial.println(z_reading);
    // Serial.println("");
    // Serial.flush();
    delayUs(1000);
    // check if any of the sensor data show movement
    if (-3000 > x_reading || 3000 < x_reading
        || -3000 > y_reading || 3000 < y_reading){
        return 1;
    }
    else{
        return 0;
    }
}