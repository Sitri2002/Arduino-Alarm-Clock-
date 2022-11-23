#ifndef I2C_H
#define I2C_H 

// Initializes the I2C module by waking it up and setting bitrate to 10kHz
void InitI2C();

// Wake up MPU chip by writing WAKEUP value to PWR_MGMT register address
void InitMPU();

// Uses the MPU readings to check for movement
int check_movement();

#endif