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
#define RADIO_ID			0x01

// radio type defaults to be receiver
Radio_t radioType = RECEIVER;
// address of receiver and transmitter
uint8_t address[] = {0x30, 0x30, 0x30, 0x31, 0x31};
// state for running the state machine
State_t currentState;
// value of LED
bool val = true;
// previous spinning direction of motor
static bool prevSpinDir = false;
// current spinning direction of motor
bool spinDir;

// reuse these arrays for sending and receiving data via SPI
uint8_t bytes[7];
uint8_t result[7];

int main(int argc, char** argv) {
    // LED configure (pin 13 - RA0)
	TRISAbits.TRISA0 = 0;
    // set as default low
	LATAbits.LATA0 = 0;
    
    // radio CE configure (pin 12 - RA1)
	TRISAbits.TRISA1 = 0;
    // set radio enable as default low
	ce(LOW);
    
    // enable RC5 as input for IRQ pin from radio
    TRISCbits.TRISC5 = 1;
    // disable analog input
    ANSELCbits.ANSC5 = 0;
    
    // receiver/transmitter selection pin (pin 2 - RA5)
	TRISAbits.TRISA5 = 1;
    // disable analog input
    ANSELAbits.ANSA5 = 0;
    // check if radio type should be transmitter
    // if RA5 is HIGH then radio is a transmitter
    // if RA5 is LOW then radio is a receiver
	if (PORTAbits.RA5) {
		radioType = TRANSMITTER;
	}
    
    // set up SPI
    setupSPI();
    
    // start radio
    bool radioStarted = true;    
    if (StartRadio(CHANNEL, PAYLOAD_SIZE)) {
        radioStarted = true;
    } else {
        // radio didn't start successfully
        // blink LED to indicate error
        while (1) {
            LATAbits.LATA0 = val;
            val = ~val;
            delay(10000);
        }
    }

    // if radio as started correctly and radio type is receiver
    // then configure radio and microcontroller as a receiver
    // otherwise configure radio and microcontroller as a transmitter
    if (radioStarted && radioType == RECEIVER) {
        configureRX(address);
        // set initial state
        currentState = READ_RX;
    } else if (radioStarted && radioType == TRANSMITTER) {
        configureTX(address);
        // set initial state
        currentState = COLLECT_ADC_DATA;
    }
    
    while (1) {
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
                        packagePayload(RADIO_ID, bytes[0], bytes[1], bytes[2], bytes[3]);
                        currentState = TRANSMIT_ADC_DATA;
                    }
                    ADCON0bits.GO_nDONE = 1;
                }
                break;
            }

            case TRANSMIT_ADC_DATA: {
                sendNOP(result);
                transmitPayload(result[0]);
                currentState = CLEAR_INTERRUPT;
                break;
            }

            case CLEAR_INTERRUPT: {
                if (!PORTCbits.RC5) {
                    sendNOP(result);
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
                LATAbits.LATA0 = 1;
//                currentState = COLLECT_ADC_DATA;
                break;
            }

            case READ_RX: {
                if (!PORTCbits.RC5) {
                    sendNOP(result);
                    if (result[0] & 0x40) {
                        currentState = UPDATE_CONTROLS;
                    }
                    uint8_t thingie[] = {W_REGISTER | SPI_STATUS, 0x70};
                    SendSPI(thingie, result, 2);
                }
                break;
            }

            case UPDATE_CONTROLS: {
                bool validData = readRXFIFO(result);
                if (validData && result[1] == RADIO_ID) {
                    uint16_t motorSpeed = (uint16_t)(((result[1] << 8) | result[2]) - 512);
                    if (motorSpeed > 512) {
                        motorSpeed *= 2;
                        spinDir = true;
                    } else {
                        motorSpeed *= (uint16_t)(-2);
                        spinDir = false;
                    }
                    PWM5CONbits.PWM5EN = 0;
                    if (spinDir != prevSpinDir) {
                        if (spinDir) {
                            RC2PPS = 0b00010;
                            LATCbits.LATC4 = 0;
                        } else {
                            RC4PPS = 0b00010;
                            LATCbits.LATC2 = 0;
                        }
                        prevSpinDir = spinDir;
                    }
                    
//                    PWM5DCL = (uint8_t)(motorSpeed << 6);
//                    PWM5DCH = (uint8_t)(motorSpeed >> 2);
                    PWM5DCL = 0xC0;
                    PWM5DCH = 0x3F;
                    PWM5CONbits.PWM5EN = 1;
//                    PWM6DCL = result[4];
//                    PWM6DCH = result[3];
//                    // clear interrupts
//                    // maybe remove this
//                    uint8_t databytes[2];
//                    databytes[0] = W_REGISTER | SPI_STATUS;
//                    databytes[1] = 0x70;
//                    SendSPI(databytes, result, 2);
                }
                currentState = DONE;
                break;
            }
        }
    }
    return 1;
}

