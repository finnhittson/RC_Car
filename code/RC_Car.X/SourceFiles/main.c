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
bool prevSpinDir = true;
// current spinning direction of motor
bool spinDir;

// reuse these arrays for sending and receiving data via SPI
uint8_t bytes[7];
uint8_t result[7];

int main(int argc, char** argv) {
    // LED/LIGHT setup
    // configure RA0 (pin 13) as digital output
	TRISAbits.TRISA0 = 0;
    // set as default low
	LATAbits.LATA0 = 0;
    
    // receiver/transmitter selection setup
    // configure RA5 (pin 2) as digital input
	TRISAbits.TRISA5 = 1;
    // disable analog input
    ANSELAbits.ANSA5 = 0;
    // check if radio type should be transmitter
    // if RA5 is HIGH then radio is a transmitter
    // if RA5 is LOW then radio is a receiver
	if (PORTAbits.RA5) {
		radioType = TRANSMITTER;
	}
    
    if (radioType == TRANSMITTER) {
        for (uint8_t i = 0; i < 10; i++) {
            delay(1000);
            LATAbits.LATA0 = 1;
            delay(1000);
            LATAbits.LATA0 = 0;
        }
    } else if (radioType == RECEIVER) {
        for (uint8_t i = 0; i < 3; i++) {
            delay(10000);
            LATAbits.LATA0 = 1;
            delay(10000);
            LATAbits.LATA0 = 0;
        }
    }
    
    // radio CE setup
    if (radioType == TRANSMITTER) {
        // configure RA2 (pin 11) as digital output
        TRISAbits.TRISA2 = 0;
    } else if (radioType == RECEIVER) {
        // configure RC5 (pin 5) as digital output
        TRISCbits.TRISC5 = 0;
    }
    // set radio enable as default low
	ce(radioType, LOW);
    
    // radio IRQ setup
    if (radioType == TRANSMITTER) {
        // configure RC2 (pin 8) as digital input
        TRISCbits.TRISC2 = 1;
        // disable analog input
        ANSELCbits.ANSC2 = 0;
    } else if (radioType == RECEIVER) {
        // configure RA1 (pint 12) as digital input
        TRISAbits.TRISA1 = 1;
        // disable analog input
        ANSELAbits.ANSA1 = 0;
    }
        
    
    // set up SPI
    setupSPI(radioType);
    
    // start radio
    bool radioStarted = true;    
    if (StartRadio(CHANNEL, PAYLOAD_SIZE, radioType)) {
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
        ADCON0bits.GO_nDONE = 1;
    }
    
    // run main state machine
    while (1) {
        switch (currentState) {
            
            // first three states are for the controller only
            // need to collect ADC data from the three potentiometers on the board
            case COLLECT_ADC_DATA: {
                if (!ADCON0bits.GO_nDONE) {
                    // cycle through the three pots
                    if (ADCON0bits.CHS == 0x14) {
                        // store high and low byte into the bytes array
                        bytes[0] = ADRESH;
                        bytes[1] = ADRESL;
                        // switch to next ADC input pin on next ADC conversion
                        ADCON0bits.CHS = 0x13;
                    } else if (ADCON0bits.CHS == 0x13) {
                        bytes[2] = ADRESH;
                        bytes[3] = ADRESL;
                        ADCON0bits.CHS = 0x01;
                    } else if (ADCON0bits.CHS == 0x01) {
                        bytes[4] = ADRESH;
                        bytes[5] = ADRESL;
                        ADCON0bits.CHS = 0x14;
                        // if the controls have changed since the last conversion
                        // then transmit; otherwise repeat the process in this state
                        if (controlsChanged(bytes)) {
                            packagePayload(RADIO_ID, bytes);
                            currentState = TRANSMIT_ADC_DATA;
                            
                        }
                    }
                    ADCON0bits.GO_nDONE = 1;
                }
                break;
            }

            // send data to nRF24L01 via SPI bus and transmit
            case TRANSMIT_ADC_DATA: {
                sendNOP(result);
                transmitPayload(result[0]);
                currentState = CLEAR_INTERRUPT;
                break;
            }
            
            // checks for interrupt to signify when transmission is done
            case CLEAR_INTERRUPT: {
                if (!PORTCbits.RC2) {
                    LATAbits.LATA0 = 1;
                    delay(1000);
                    LATAbits.LATA0 = 0;
                    delay(1000);
                    // get interrupt type 
                    sendNOP(result);
                    // clear interrupt so nRF24L01 can transmit again
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

            // starts next ADC conversion
            case DONE: {
                currentState = COLLECT_ADC_DATA;
                ADCON0bits.GO_nDONE = 1;
                break;
            }

            // final states are for the receiver
            // read data from the nRF24L01 when it signifies there is data ready
            case READ_RX: {
                // checks for IRQ line to be low
                if (!PORTAbits.RA1) {
                    LATAbits.LATA0 = 1;
                    delay(1000);
                    LATAbits.LATA0 = 0;
                    delay(1000);
                    // once low checks for type of interrupt
                    sendNOP(result);
                    // if interrupt is data ready (0x40) then begin data collection process
                    if (result[0] & 0x40) {
                        currentState = UPDATE_CONTROLS;
                    }
                    // clear interrupts
                    bytes[0] = W_REGISTER | SPI_STATUS;
                    bytes[1] = 0x70;
                    SendSPI(bytes, result, 2);
                }
                break;
            }
            
            // gets data from nRF24L01 and updates servo and motor
            case UPDATE_CONTROLS: {
                // collect data from nRF24L01 and check if validity
                bool validData = readRXFIFO(result);
                // if data is uncorrupted and coming from the right source then update
                if (validData && result[1] == RADIO_ID) {
                    // convert motor and servo data to proper data type
                    uint16_t motorSpeed = (uint16_t)((result[2] << 8) | result[3]);
                    uint16_t servoPos = (uint16_t)((result[4] << 8) | result[5]);
                    // convert servo data to a 1ms to 2ms format
                    servoPos = (servoPos * 15 + 15345) / 1023;
                    // convert motor data such that 0 is full reverse, 512 is 
                    // stopped, and 1023 is full forward
                    if (motorSpeed > 512) {
                        motorSpeed -= 512;
                        spinDir = true;
                    } else {
                        motorSpeed = 512 - motorSpeed;
//                        motorSpeed = 0;
                        spinDir = false;
                    }
                    // disable motor pwm line
                    PWM5CONbits.PWM5EN = 0;
                    // check if motor spin direction has changed
                    if (prevSpinDir != spinDir) {
                        // if changed then swap pin mapping
                        if (spinDir) {
                            RC2PPS = 0b00010;
                            RC4PPS = 0b00000;
                            LATCbits.LATC4 = 0;
                        } else {
                            RC2PPS = 0b00000;
                            RC4PPS = 0b00010;
                            LATCbits.LATC2 = 0;
                        }
                        // update motor spinning direction
                        prevSpinDir = spinDir;
                    }
                    
                    // update motor PWM speed
                    PWM5DCL = (uint8_t)(motorSpeed << 6);
                    PWM5DCH = (uint8_t)(motorSpeed >> 2);
                    
                    // update servo PWM speed
                    PWM6DCL = (uint8_t)(servoPos << 6);
                    PWM6DCH = (uint8_t)(servoPos >> 2);
                    
                    // re-enable motor PWM
                    PWM5CONbits.PWM5EN = 1;
                }
                // go back to READ_RX state and wait for next command to be ready
                currentState = READ_RX;
                break;
            }
        }
    }
    return 1;
}

