/* 
 * File:   uart.h
 * Author: mattia
 *
 * Created on April 4, 2025, 2:42 PM
 */

#ifndef UART_H
#define	UART_H

#include <xc.h>

#define INPUT_BUFF_LEN 50   //TODO: SET THE CORRECT SIZE
#define OUTPUT_BUFF_LEN 50  //TODO: SET THE CORRECT SIZE

extern int int_ret; 

struct input_buffer {
    char buff[INPUT_BUFF_LEN];
    int read;
    int write;
    int new_data;
};

extern struct input_buffer UART_input_buff; 
struct output_buffer {
    char buff[OUTPUT_BUFF_LEN];
    int read;
    int write;
    int new_data;
};

extern struct output_buffer UART_output_buff; 

void init_uart();
void print_to_buff(const char * str);

#endif	/* UART_H */


