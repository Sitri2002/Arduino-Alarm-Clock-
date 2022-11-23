#include <avr/io.h>
#include <avr/interrupt.h>
#include "remote.h"

// State machine for receiving data from remote. wait_burst and wait_space are 
// used to check for the start of a message (idle states) whereas reading is where 
// the data is read
typedef enum receiveState_enum {
    wait_burst, wait_space, reading}
receiveState_enum;
// State of the state machine for receiving data from remote. Initially in wait_burst
volatile receiveState_enum receive_state = wait_burst;
// counter used to check for repeated high or low inputs from IR remote sensor
volatile int my_counter = 0;

// message received from remote
volatile char remote_data[100];
// end of message received from remote
volatile int remote_data_end = 0;
// flag for if the remote data is ready
volatile int remote_data_available = 0;

// Initialize remote with its timer (timer 3) for receiving IR NEC signals.
void init_remote(){

    // Setting timer 3 into CTC mode
    TCCR3B |= ( 1 << WGM32); 

    // Setting compare A register to 8999 = (562.5*16/1) - 1 to count for 562.5 us.
    // given a prescaler of 1
    OCR3A = 8999;

    DDRH |= (1 << DDH3);
    DDRE |= (1 << DDE3);
    //PORTE |= (1 << PORTE3);
    PORTH |= (1 << PORTH3);

    // Sets timer 3 prescaler to 1024
    TCCR3B  |= (1 << CS30);
    
    // Enables interrupt on timer 3 compare A
    TIMSK3 |= (1 << OCIE3A);
}

// converts binary string given in binary_string to 8-bit int
uint8_t binary_string_to_uint8_t(const char * binary_string){

    uint8_t value = 0;
    for(int k = 0; k < 8; k++){
        if(binary_string[k] == '1')
            value += (1 << (7-k));
    }
    return value;
}

// Get input data from remote and return the button pressed. 
// 'E' indicates communication error.
char get_remote_input(){

    // wait for remote input data to be available
    if(!remote_data_available)
        return 0;
    
    // select 8 digits of flipped button_code and convert it to a number and remove it 
    // from remote data.
    remote_data[remote_data_end] = '\0';
    const char* button_code_flipped_string = (const char*)(&remote_data[remote_data_end-8]);
    uint8_t button_code_flipped = binary_string_to_uint8_t(button_code_flipped_string);
    remote_data_end -= 8;

    // select 8 digits for button code and convert it to a number
    remote_data[remote_data_end] = '\0';
    const char* button_code_string = (const char*)(&remote_data[remote_data_end-8]);
    uint8_t button_code = binary_string_to_uint8_t(button_code_string);
    remote_data_end -= 8;
    
    // Flag remote input data as used so it gets fetched again
    remote_data_available = 0;

    // Compare command with its flipped version to check for communication errors
    if(button_code_flipped != (uint8_t)(~button_code))
        return 'E';
    else
        // return the button corresponding to the command number from the remote
        return button_code_to_button(button_code);
}

// convert a command number given in command_num from the remote to the button 
// in question. '?' indicates unused or unknown button.
char button_code_to_button(uint8_t button_code){
    switch(button_code){
        case 0x68:
            return '0';
        case 0x30:
            return '1';
        case 0x18:
            return '2';
        case 0x7A:
            return '3';
        case 0x10:
            return '4';
        case 0x38:
            return '5';
        case 0x5A:
            return '6';
        case 0x42:
            return '7';
        case 0x4A:
            return '8';
        case 0x52:
            return '9';
        case 0x62:
            return 'M';
        case 0xE2:
            return 'S';
        case 0xC2:
            return 'R';
        case 0x02:
            return 'L';
        case 0x98:
            return '/';
        case 0xA2:
            return 'A';
        case 0xA8:
            return 'P';
        default:
            return '?';
    }
}

// Interrupt to decode data from remote using a state machine. Data from remote is given 
// following the NEC protocol.
ISR(TIMER3_COMPA_vect){

    // Don't start decoding messages until previous message has been consumed
    if(remote_data_available)
        return;

    // if pin input is low then data input is high and vice versa because the NEC 
    // protocol has input be high by default.
    int pin_data;
    //if(PINF & (1 << PINF0))
    if(PINH & (1 << PINH4))
        pin_data = 0;
    else{
        pin_data = 1;
    }

    // Wait for NEC protocol burst by waiting for at least 200 low ticks then a high tick
    if(receive_state == wait_burst){
        // If low ticks reached 200 and a high tick is detected, move to wait_space
        if(pin_data && my_counter > 200){
            my_counter = 0;
            receive_state = wait_space;
        }
        // Count up low ticks (target is above 200)
        else if ((!pin_data) && (my_counter < 300)){
            my_counter += 1;
        }
    }

    // Wait for NEC protocol space by waiting for at least 20 ticks then checking for a high tick 
    // within the next 20 ticks
    else if(receive_state == wait_space){

        my_counter += 1;
        // Wait for 20 ticks then check for a high tick to move to reading stage (start of message)
        if(pin_data && my_counter > 20){
            my_counter = 0;
            remote_data_end = 0;
            receive_state = reading;
        }
        // If no high tick after more than 40 ticks, return to wait_burst stage
        else if((!pin_data) && my_counter > 40){
            my_counter = 0;
            receive_state = wait_burst;
        }
    }
    
    // Reading stage decodes message from high and low ticks to bits. one high tick 
    // followed by 1 low tick is a 0 bit. One high tick followed by 3 low ticks is a 1 
    // bit. The bits are stored in remote_data.
    else if(receive_state == reading){
        // For each high tick, check how many low ticks were before it.
        if(pin_data){
            // One low tick before means 0 bit
            if(my_counter <= 2){
                remote_data[remote_data_end] = '0';
            }
            // Three low ticks before means 1 bit
            else{
                remote_data[remote_data_end] = '1';
            }
            remote_data_end += 1;
            my_counter = 0;
        }
        // Count low ticks since last high tick
        else{
            my_counter += 1;
        }
    
        // After 50 low ticks, message is considered over. Return to wait_burst stage 
        // and signal that remote data is available.
        if(my_counter > 50){
            my_counter = 0;
            remote_data_available = 1;
            receive_state = wait_burst;
        }
    }
    
}

