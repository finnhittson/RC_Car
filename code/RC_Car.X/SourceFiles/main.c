#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include "stdbool.h"
#include "../HeaderFiles/TransmitService.h"

#pragma config WDTE = OFF           // Watchdog Timer Enable bit (WDT disabled)
#pragma config FEXTOSC = OFF        // External Oscillator mode selection
#pragma config RSTOSC = HFINT1      // Power-up default value for COSC (HFINTOSC with HFFRQ = 1 MHz and CDIV = 1:1)
#pragma config WDTE = OFF           // Watchdog Timer Enable
#pragma config MCLRE = ON           // Master Clear Enable

#define CHANNEL				42
#define ADDRESS_WIDTH		5

Radio_t radioType = RECEIVER;
SPISTATUSbits_t SPI_STATUSbits;
uint8_t address[] = {0x30, 0x30, 0x30, 0x31, 0x31};

int main(int argc, char** argv) {
    // LED configure (pin 13 - RA0)
	TRISAbits.TRISA0 = 0;
	LATAbits.LATA0 = 0;
    
    // radio CE configure (pin 12 - RA1)
	TRISAbits.TRISA1 = 0;
	ce(LOW);
    
    // receiver/transmitter selection pin (pin 2 - RA5)
	TRISAbits.TRISA5 = 1;
    ANSELAbits.ANSA5 = 0;
	if (PORTAbits.RA5) {
		radioType = TRANSMITTER;
	}
    
    // enable interrupts
	if (true) {
		TRISCbits.TRISC5 = 1;			// enable interrupt pin as input
        ANSELCbits.ANSC5 = 0;           // disable analog input
		IOCCNbits.IOCCN5 = 1;           // interrupt on change enabled on pin 2 (RA5) for negative going edge
	}
    
    // SPI configure
	if (true) {
		// SCLK configure (RC0 - pin 10)
        TRISCbits.TRISC0 = 0;
        // map RC0 to SPI SCLK
        RC0PPS = 0b11000;

        // MISO/SDI configure (RC1 - pin 9)
        TRISCbits.TRISC1 = 1;
        ANSELCbits.ANSC1 = 0;
        // map RC1 to SPI MISO
        SSP1DATPPS = 0b10001;

        // MOSI/SDO configure (RA2 - 11)
        TRISAbits.TRISA2 = 0;
        // map RA2 to MOSI
        RA2PPS = 0b11001;

        // CS configure (RC2 - pin 8)
        TRISCbits.TRISC2 = 0;
        // set CS line high by default
        LATCbits.LATC2 = 1;
        
        // input data sampled at middle of data output time
        SSP1STATbits.SMP = 1;
        // transmit occurs on transition from active to Idle clock state
        SSP1STATbits.CKE = 1;
        // enables serial port and configures SCK, SDO, SDI and SS as the source of the serial PORT pins
        SSP1CON1bits.SSPEN = 1;
        // idle state for clock is a low level
        SSP1CON1bits.CKP = 0;
        // SPI Master mode, clock = FOSC/4
        SSP1CON1bits.SSPM = 0;
	}
    
    // start radio
	bool radioStarted = false;
	uint8_t result[2];
	if (StartRadio(CHANNEL, PAYLOAD_SIZE, result)) {
		radioStarted = true;
	}
    SPI_STATUSbits.w = result[0];
    
    if (radioStarted && radioType == RECEIVER) {
        LATAbits.LATA0 = 1;
		// open reading pipe
		uint8_t databytes[2];
        uint8_t result[2];
        databytes[0] = W_REGISTER | EN_RXADDR;
        databytes[1] = 0x01;
        SendSPI(databytes, result, 2);
		SPI_STATUSbits.w = result[0];

		// setup data pipe for receiving data
		StartListening(address, ADDRESS_WIDTH, result);
		SPI_STATUSbits.w = result[0];

		// clear status register
        databytes[0] = W_REGISTER | SPI_STATUS;
		databytes[1] = 0x70;
        SendSPI(databytes, result, 2);
		SPI_STATUSbits.w = result[0];

        // configure timer 2 for PWM channels
        T2CONbits.TMR2ON = 0;       // turn off timer 2
        T2CONbits.T2CKPS = 0b01;    // 1:4 pre scaler
        TMR2 = 0;                   // clear timer 2 counter bits
        PR2 = 125;                  // set timer 2 period 125us
        T2CONbits.TMR2ON = 1;       // turn on timer 2
        
        // configure PMW5 module
        TRISCbits.TRISC3 = 0;       // set RC3 as output
        PWM5CONbits.PWM5EN = 0;     // disable PWM5 module
        PWM5CONbits.PWM5POL = 0;    // PWM5 output is active-high
        RC3PPS = 0b00010;           // map RC3 to PWM5
        PWM5DCL = 0xC0;             // set duty cycle to 10
        PWM5DCH = 0x6F;             // set duty cycle to 10
        PWMTMRSbits.P5TSEL = 0b01;  // select timer 2 as timing source
        PWM5CONbits.PWM5EN = 1;     // enable PWM5 module
        
        // configure PWM6 module
        TRISCbits.TRISC4 = 0;       // set RC4 as output
        PWM6CONbits.PWM6EN = 0;     // disable PWM5 module
        PWM6CONbits.PWM6POL = 0;    // PWM5 output is active-high
        RC4PPS = 0b00011;           // map RC4 to PWM6
        PWM6DCL = 0x00;             // set duty cycle to 10
        PWM6DCH = 0x00;             // set duty cycle to 10
        PWMTMRSbits.P6TSEL = 0b01;  // select timer 2 as timing source
        PWM6CONbits.PWM6EN = 1;     // enable PWM5 module
	}
    
    while (1) {
        
    }
    
    return 1;
}

