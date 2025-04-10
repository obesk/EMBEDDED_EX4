/*
 * File:   main.c
 * Author: obe
 *
 * Created on March 17, 2025, 10:06 AM
 */

#include "timer.h"
#include "string.h"

#include "xc.h"


// 100Hz (the while frequency) / 2.5Hz (the LD2 frequency)
#define CLOCK_LD_TOGGLE 40 
#define MAX_INT_LEN 2
#define BUFF_LEN 10

int ld2_blink = 0;
int UART_chars_n = 0;
int missed_deadlines = 0;

int print_n_chars = 0;
int print_missed_deadlines = 0;


struct circular_buffer {
    char buff[BUFF_LEN];
    int read;
    int write;
    int new_data;
};

struct circular_buffer input_buff; 

void algorithm() {
    tmr_wait_ms(TIMER2, 7);
}

void init_buttons() {
    TRISA = TRISG = ANSELA = ANSELB = ANSELC = ANSELD = ANSELE = ANSELG = 0x0000;
    
    RPINR0bits.INT1R = 0x58; // remapping the interrupt 1 to the T2 button pin
    IFS1bits.INT1IF = 0; // resetting flag of interrupt 1
    
    RPINR1bits.INT2R = 0x59; // remapping the interrupt 2 to the T3 button pin
    IFS1bits.INT2IF = 0; // resetting flag of interrupt 2
    
    IEC1bits.INT1IE = 1; // enabling interrupt 1
    IEC1bits.INT2IE = 1; // enabling interrupt 2
}

void init_uart() {
    RPINR18bits.U1RXR = 0b1001011; // mapping pin RD11(RPI75) to UART RX
    RPOR0bits.RP64R = 0b000001; // mapping pin RD0(RP64) to UART TX

    U1BRG = 467; // baud rate to 9600 -> 72 000 000 / (16 * 9600) - 1

    U1STAbits.URXISEL = 0; // set to interrupt on char received
    U1MODEbits.UARTEN = 1; // enable UART
    U1STAbits.UTXEN = 1; // enable UART transmission
    
    IFS0bits.U1RXIF = 0; // interrupt flag set to 0
    IEC0bits.U1RXIE = 1; // enabled interrupt on receive
}

void print_to_uart(const char * str) {
    if(!str) {
        return; 
    }
    
    for (int i = 0; str[i] != '\0'; ++i) {
        while(U1STAbits.UTXBF);
        U1TXREG = str[i];
    }
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
    init_buttons();
    init_uart();
    INTCON2bits.GIE = 1; // global interrupt enable
    
    char output_buff[10]; //TODO: change size

    
    int LD2_toggle_counter = 0;
    
    tmr_setup_period(TIMER1, 10);
    while(1) {
        algorithm();
        ++LD2_toggle_counter;
        
        while(input_buff.read != input_buff.write) {
            match_string(input_buff.buff[input_buff.read]);
            input_buff.read = (input_buff.read + 1) % BUFF_LEN;
        }
        
        if (print_missed_deadlines) {
            print_missed_deadlines = 0;
            
            sprintf(output_buff, "C=%d", UART_chars_n);
            print_to_uart(output_buff);
        }
        
        if (print_n_chars) {
            print_n_chars = 0;
            
            sprintf(output_buff, "D=%d", missed_deadlines);
            print_to_uart(output_buff);

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
        U1TXREG = read_char;

        input_buff.buff[input_buff.write] = read_char;
        input_buff.write = (input_buff.write + 1) % BUFF_LEN;
    }
}

void __attribute__((__interrupt__, __auto_psv__)) _INT1Interrupt(){
    tmr_setup_period(TIMER2, 2);
    IFS1bits.INT1IF = 0;
    print_n_chars = 1;
    tmr_wait_period(TIMER2);
}

void __attribute__((__interrupt__, __auto_psv__)) _INT2Interrupt(){
    tmr_setup_period(TIMER2, 2);
    IFS1bits.INT2IF = 0;
    print_missed_deadlines = 1;
    tmr_wait_period(TIMER2);
}
