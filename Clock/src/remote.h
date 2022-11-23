#ifndef REMOTE_H
#define REMOTE_H 

// Initialize remote with its timer (timer 3) for receiving IR NEC signals.
void init_remote();

// Get input data from remote and return the button pressed. 
// 'E' indicates communication error.
char get_remote_input();

// convert a command number given in command_num from the remote to the button 
// in question. '?' indicates unused or unknown button.
char button_code_to_button(uint8_t button_code);

#endif

