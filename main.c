/*
 * File:   main.c
 * Author: obe
 *
 * Created on March 17, 2025, 10:06 AM
 */

#include "timer.h"
#include "string.h"
#include "uart.h"

#include "xc.h"


// 100Hz (the while frequency) / 2.5Hz (the LD2 frequency)
#define CLOCK_LD_TOGGLE 40 
#define MAX_INT_LEN 2

int ld2_blink = 0;
int UART_chars_n = 0;
int missed_deadlines = 0;

int print_n_chars = 0;
int print_missed_deadlines = 0;

void algorithm() {
    tmr_wait_ms(TIMER2, 7);
}

void init_buttons() {
    
    RPINR0bits.INT1R = 0x58; // remapping the interrupt 1 to the T2 button pin
    IFS1bits.INT1IF = 0; // resetting flag of interrupt 1
    
    RPINR1bits.INT2R = 0x59; // remapping the interrupt 2 to the T3 button pin
    IFS1bits.INT2IF = 0; // resetting flag of interrupt 2
    
    IEC1bits.INT1IE = 1; // enabling interrupt 1
    IEC1bits.INT2IE = 1; // enabling interrupt 2
}

void match_string(char c) {
    static int str_crs = 0; 
    static const char *LD1 = "LD1";
    static const char *LD2 = "LD2";

    if (c == LD1[str_crs] || c == LD2[str_crs]) {
        ++str_crs;
        if ((str_crs == 3) && (c == LD1[str_crs - 1])) { // matched the LD1 string
            LATA = !LATA;
            str_crs = 0;
        }
        if ((str_crs == 3) && (c == LD2[str_crs - 1])) { //matched the LD2 string
            ld2_blink = !ld2_blink;
            str_crs = 0;
        }
    } else if (c == LD1[0] || c == LD2[0]){
        str_crs = 1;
    } else {
        str_crs = 0;
    }
}


int main(void) {
    char input_buff[INPUT_BUFF_LEN];
    char output_buff[OUTPUT_BUFF_LEN];
 
    UART_input_buff.buff = input_buff;
    UART_output_buff.buff = output_buff;

    init_buttons();
    init_uart();


    char output_str [50];

    TRISA = TRISG = ANSELA = ANSELB = ANSELC = ANSELD = ANSELE = ANSELG = 0x0000;

    INTCON2bits.GIE = 1; // global interrupt enable
    
    int LD2_toggle_counter = 0;
    
    tmr_setup_period(TIMER1, 10);

    while(1) {
        algorithm();
        ++LD2_toggle_counter;
        
        
        while(UART_input_buff.read != UART_input_buff.write) {
            match_string(UART_input_buff.buff[UART_input_buff.read]);
            UART_input_buff.read = (UART_input_buff.read + 1) % INPUT_BUFF_LEN;
        }
        
        if (print_missed_deadlines) {
            print_missed_deadlines = 0;
            
            sprintf(output_str, "D=%d", missed_deadlines);
            print_to_buff(output_str);
        }
        
        if (print_n_chars) {
            print_n_chars = 0;
            
            sprintf(output_str, "C=%d", UART_chars_n);
            print_to_buff(output_str);

        }
 
        if (LD2_toggle_counter >= CLOCK_LD_TOGGLE) {
            LD2_toggle_counter = 0;
            LATGbits.LATG9 = !LATGbits.LATG9 && ld2_blink;
        }
        
        missed_deadlines += tmr_wait_period(TIMER1);
    }
    return 0;
}

void __attribute__((__interrupt__)) _U1RXInterrupt(void) {
    IFS0bits.U1RXIF = 0; //resetting the interrupt flag to 0
 
    while(U1STAbits.URXDA) {
        ++UART_chars_n; 
        const char read_char = U1RXREG;

        const int new_write_index = (UART_input_buff.write + 1) % INPUT_BUFF_LEN;
        if (new_write_index != UART_input_buff.read) {
            UART_input_buff.buff[UART_input_buff.write] = read_char;
            UART_input_buff.write = new_write_index;
        }
    }
}

void __attribute__((__interrupt__, __auto_psv__)) _INT1Interrupt(void){
    IFS1bits.INT1IF = 0;
    print_n_chars = 1;
}

void __attribute__((__interrupt__, __auto_psv__)) _INT2Interrupt(void){
    IFS1bits.INT2IF = 0;
    print_missed_deadlines = 1;
}

void __attribute__((__interrupt__)) _U1TXInterrupt(void){

    IFS0bits.U1TXIF = 0; // clear TX interrupt flag
    if(UART_output_buff.read == UART_output_buff.write){
        int_ret = 1;
    }

    while(!U1STAbits.UTXBF && UART_output_buff.read != UART_output_buff.write){
        U1TXREG = UART_output_buff.buff[UART_output_buff.read];
        UART_output_buff.read = (UART_output_buff.read + 1) % OUTPUT_BUFF_LEN;
    }
}