#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include "stdbool.h"
#include "../HeaderFiles/TransmitService.h"
#include "string.h"

#pragma config WDTE = OFF           // Watchdog Timer Enable bit (WDT disabled)
#pragma config FEXTOSC = OFF        // External Oscillator mode selection
#pragma config RSTOSC = HFINT1      // Power-up default value for COSC (HFINTOSC with HFFRQ = 1 MHz and CDIV = 1:1)
#pragma config WDTE = OFF           // Watchdog Timer Enable
#pragma config MCLRE = ON           // Master Clear Enable

#define CHANNEL				42
#define ADDRESS_WIDTH		5

Radio_t radioType = RECEIVER;
uint8_t address[] = {0x30, 0x30, 0x30, 0x31, 0x31};
State_t currentState;
uint8_t bytes[6];
uint8_t result[2];

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
//		IOCCNbits.IOCCN5 = 1;           // interrupt on change enabled on pin 2 (RA5) for negative going edge
	}
    
    // SPI configure
	if (true) {
		// SCLK config
        TRISCbits.TRISC0 = 0;
        RC0PPS = 0b11000;

        // MISO/SDI config
        TRISCbits.TRISC1 = 1;
        ANSELCbits.ANSC1 = 0;
        SSP1DATPPS = 0b10001;

        // MOSI/SDO config
        TRISAbits.TRISA4 = 0;
        RA4PPS = 0b11001;

        // CS configure
        TRISCbits.TRISC3 = 0;
        LATCbits.LATC3 = 1;

        // enables serial port and configures SCK, SDO, SDI and SS as the source of the serial PORT pins
        SSP1CON1bits.SSPEN = 1;
        // SPI Master mode, clock = FOSC/4
        SSP1CON1bits.SSPM = 0b0000;
        // input data sampled at middle of data output time
        SSP1STATbits.SMP = 1;   // this bit needs to be set otherwise SPI doesn't work
        // CLOCK PHASE: transmit occurs on transition from active to Idle clock state
        SSP1STATbits.CKE = 1;
        // CLOCK POLARITY: clock idles low
        SSP1CON1bits.CKP = 0;
	}
    
    // start radio
	bool radioStarted = true;    
	if (StartRadio(CHANNEL, PAYLOAD_SIZE)) {
		radioStarted = true;
	} else {
        bool val = true;
        while (1) {
            LATAbits.LATA0 = val;
            val = ~val;
            delay(1000);
        }
    }
    
    if (radioStarted && radioType == RECEIVER) {
        // LATAbits.LATA0 = 1;
		// open reading pipe
		uint8_t databytes[2];
        uint8_t result[2];
        databytes[0] = W_REGISTER | EN_RXADDR;
        databytes[1] = 0x01;
        SendSPI(databytes, result, 2);

		// setup data pipe for receiving data
		StartListening(address);

		// clear status register
        databytes[0] = W_REGISTER | SPI_STATUS;
		databytes[1] = 0x70;
        SendSPI(databytes, result, 2);

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
    
    // setup radio as transmitter
	if (radioStarted && radioType == TRANSMITTER) {
		// set address
        uint8_t databytes[6];
		uint8_t result[6];
        databytes[0] = W_REGISTER | RX_ADDR_P0;
        databytes[1] = address[0];
        databytes[2] = address[1];
        databytes[3] = address[2];
        databytes[4] = address[3];
        databytes[5] = address[4];
        SendSPI(databytes, result, ADDRESS_WIDTH + 1);
        databytes[0] = W_REGISTER | TX_ADDR;
        SendSPI(databytes, result, ADDRESS_WIDTH + 1);

		// power up radio
		ChangeRadioMode(Standby1, 1);

		// configure analog input
        ANSELCbits.ANSC4 = 1;
		TRISCbits.TRISC4 = 1;
        
        ADCON0bits.ADON = 0;        // turn off ADC
        ADCON1bits.ADCS = 0b110;    // select ADC conversion clock as Fosc/2
        ADCON1bits.ADNREF = 0;      // set negative voltage reference as Vss
        ADCON1bits.ADPREF = 0;      // set positive voltage reference as Vdd
        ADCON0bits.CHS = 0x14;      // select analog input channel as RC4
        ADCON1bits.ADFM = 0;        // result is stored left justified
        ADCON0bits.ADON = 1;        // turn on ADC
        
        currentState = COLLECT_ADC_DATA;
    }
    
    bool val = true;
    uint8_t bytes[4];
    while (1) {
        if (radioType == TRANSMITTER) {
            switch (currentState) {
                case COLLECT_ADC_DATA: {
                    if (!ADCON0bits.GO_nDONE) {
                        if (ADCON0bits.CHS == 0x14) {
                            bytes[0] = ADRESH;
                            bytes[1] = ADRESL;
                            ADCON0bits.CHS = 0x12;
                        } else {
                            bytes[2] = ADRESH;
                            bytes[3] = ADRESL;
                            ADCON0bits.CHS = 0x14;
                            currentState = TRANSMIT_ADC_DATA;
                        }
                        ADCON0bits.GO_nDONE = 1;
                    }
                    break;
                }
                
                case TRANSMIT_ADC_DATA: {
                    packagePayload(bytes[0], bytes[1], bytes[2], bytes[3]);
                    sendNOP(result);
                    transmitPayload(result[0]);
                    currentState = CLEAR_INTERRUPT;
                    break;
                }
                
                case CLEAR_INTERRUPT: {
                    if (!PORTCbits.RC5) {
                        bytes[0] = W_REGISTER | SPI_STATUS;
                        bytes[1] = 0x70;
                        while (1) {
                            SendSPI(bytes, result, 2);
                            sendNOP(result);
                            if (!(result[0] & 0x70)) {
                                break;
                            }
                        }
                        currentState = DONE;
                    }
                    break;
                }
                
                case DONE: {
                    LATAbits.LATA0 = val;
                    val = ~val;
                    currentState = COLLECT_ADC_DATA;
                    break;
                }
            }
        } else {
            // do nothing
        }
    }
    return 1;
}

