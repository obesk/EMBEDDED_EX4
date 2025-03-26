/*
 * File:   main.c
 * Author: obe
 *
 * Created on March 17, 2025, 10:06 AM
 */

#include "xc.h"
#include "timer.h"

// 100Hz (the while frequency) / 2.5Hz (the LD2 frequency)
#define CLOCK_LD_TOGGLE 40 
#define MAX_INT_LEN 2

int ld2_blink = 0;
int UART_chars_n = 0;
int missed_deadlines = 0;

void algorithm() {
    tmr_wait_ms(TIMER2, 7);
}



int main(void) {
    // enabling the led output
    ANSELA = ANSELB = ANSELC = ANSELD = ANSELE = ANSELG = 0x0000;
    TRISA = 0;
    TRISG = 0;
    
    RPINR0bits.INT1R = 0x58; // remapping the interrupt 1 to the T2 button pin
    IFS1bits.INT1IF = 0; // resetting flag of interrupt 1
    
    RPINR1bits.INT2R = 0x59; // remapping the interrupt 2 to the T3 button pin
    IFS1bits.INT2IF = 0; // resetting flag of interrupt 2
    
    INTCON2bits.GIE = 1; // global interrupt enable
    IEC1bits.INT1IE = 1; // enabling interrupt 1
    IEC1bits.INT2IE = 1; // enabling interrupt 2
    
    
    RPINR18bits.U1RXR = 0b1001011; // mapping pin RD11(RPI75) to UART RX
    RPOR0bits.RP64R = 0b000001; // mapping pin RD0(RP64) to UART TX

    U1BRG = 467; // baud rate to 9600 -> 72 000 000 / (16 * 9600) - 1

    U1STAbits.URXISEL = 0; // set to interrupt on char received
    U1MODEbits.UARTEN = 1; // enable UART
    U1STAbits.UTXEN = 1; // enable UART transmission
    
    IFS0bits.U1RXIF = 0; // interrupt flag set to 0
    INTCON2bits.GIE = 1; // global interrupt enable
    IEC0bits.U1RXIE = 1; // enabled interrupt on receive
    
    int LD2_toggle_counter = 0;
    
    tmr_setup_period(TIMER1, 10);
    while(1) {
        algorithm();
        ++LD2_toggle_counter;
        if (LD2_toggle_counter >= CLOCK_LD_TOGGLE) {
            LD2_toggle_counter = 0;
            LATGbits.LATG9 = !LATGbits.LATG9 && ld2_blink;
        }
        
        missed_deadlines += tmr_wait_period(TIMER1);
    }
    return 0;
}

int str_crs = 0; 

void __attribute__((__interrupt__)) _U1RXInterrupt(void) {
    IFS0bits.U1RXIF = 0; //resetting the interrupt flag to 0
    
    ++UART_chars_n; 

    static const char *LD1 = "LD1";
    static const char *LD2 = "LD2";
    
    //U1TXREG = U1RXREG;
    const char read_char = U1RXREG;
    
    U1TXREG = read_char;

    if (read_char == LD1[str_crs] || read_char == LD2[str_crs]) {
        ++str_crs;
        if ((str_crs == 3) && (read_char == LD1[str_crs - 1])) { // matched the LD1 string
            LATA = !LATA;
            str_crs = 0;
        }
        if ((str_crs == 3) && (read_char == LD2[str_crs - 1])) { //matched the LD2 string
            ld2_blink = !ld2_blink;
            str_crs = 0;
        }
    } else if (read_char == LD1[0] || read_char == LD2[0]){
        str_crs = 1;
    } else {
        str_crs = 0;
    }

}

void print_int_to_uart(int v) {
    static char rev_str [MAX_INT_LEN];
    int crs = 0;
    
    if (!v) {
        U1TXREG = '0';
    }
    
    if (v < 0) {
        v = -v;
        U1TXREG = '-';
        ++crs;
    }
    
    
    while (v) {
        rev_str[crs++] = '0' + (v % 10);
        v /= 10;
    }
    
    for (int i = crs - 1; i >= 0; --i) {
        U1TXREG = rev_str[i];
    }
}

void __attribute__((__interrupt__, __auto_psv__)) _INT1Interrupt(){
    IFS1bits.INT1IF = 0;
    U1TXREG = '\n';
    U1TXREG = 'C';
    U1TXREG = '=';
    print_int_to_uart(UART_chars_n);
}

void __attribute__((__interrupt__, __auto_psv__)) _INT2Interrupt(){
    IFS1bits.INT2IF = 0;
    U1TXREG = '\n';
    U1TXREG = 'D';
    U1TXREG = '=';
    print_int_to_uart(missed_deadlines);
}
