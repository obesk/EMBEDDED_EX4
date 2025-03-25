/*
 * File:   main.c
 * Author: obe
 *
 * Created on March 17, 2025, 10:06 AM
 */

#include "xc.h"

int main(void) {
    RPINR18bits.U1RXR = 0b1001011; // mapping pin RD11(RPI75) to UART RX
    RPOR0bits.RP64R = 0b000001; // mapping pin RD0(RP64) to UART TX

    U1BRG = 467; // baud rate to 9600 -> 72 000 000 / (16 * 9600) - 1

    U1STAbits.URXISEL = 0; // set to interrupt on char received
    U1MODEbits.UARTEN = 1; // enable UART
    U1STAbits.UTXEN = 1; // enable UART transmission
    
    IFS0bits.U1RXIF = 0; // interrupt flag set to 0
    INTCON2bits.GIE = 1; // global interrupt enable
    IEC0bits.U1RXIE = 1; // enabled interrupt on receive
    
    while (1);

    return 0;
}

void __attribute__((__interrupt__)) _U1RXInterrupt(void) {
    IFS0bits.U1RXIF = 0;
    U1TXREG = U1RXREG;
}
