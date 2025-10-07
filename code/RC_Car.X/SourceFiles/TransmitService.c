/*----------------------------- Include Files -----------------------------*/
#include <xc.h>
#include "../HeaderFiles/TransmitService.h"
#include "stdbool.h"

/*----------------------------- Module Defines ----------------------------*/
#define ADDRESS_WIDTH		5

/*---------------------------- Module Variables ---------------------------*/
// array for bytes to transmit
uint8_t payload[PAYLOAD_SIZE + 1];
// variable to set if radio is a receiver or a transmitter depending on RA5
Radio_t radioType = RECEIVER;

// sends contents in payload variable to nRF24L01 to be sent to receiver
void transmitPayload(uint8_t statusBits) {
    // checks first that no interrupt flags are set
	if (statusBits & 0x70) {
        // if interrupt flag set then need to clear it for the nRF24L01 to transmit
		uint8_t databytes[2];
        uint8_t result[2];
        databytes[0] = W_REGISTER | SPI_STATUS;
        databytes[1] = 0x70;
        SendSPI(databytes, result, 2);
	}
    // send payload variable to nRF24L01 radio
	uint8_t result[PAYLOAD_SIZE + 1];
	SendSPI(payload, result, PAYLOAD_SIZE + 1);
    // pulse CE line on nRF24L01 high and low to enable transmission
	ce(TRANSMITTER, HIGH);
	delay(TX_DELAY);
	ce(TRANSMITTER, LOW);
}

// puts bytes to send into payload variable
void packagePayload(uint8_t radioID, uint8_t *bytes) {
    // first compute checksum
	uint8_t checksum = 0xFF - radioID;
    for (uint8_t i = 0; i < 6; i++) {
        checksum -= bytes[i];
    }
    
    // by default first two bytes will always constant.
    // byte 1: tells the nRF24L01 that it is receiving a payload to transmit
    // byte 2: hold the ID of the radio; this should never change
	payload[0] = W_TX_PAYLOAD;
	payload[1] = radioID;
    
    // load the rest of the payload and checksum into the payload variable
	for (uint8_t i = 2; i < 8; i++) {
        payload[i] = bytes[i-2];
    }
    payload[8] = checksum;
}

// function that executes the SPI setup and initialization
void setupSPI(Radio_t radioType) {
    // SCLK config
    // configure RC0 (pin 10) as digital output
    TRISCbits.TRISC0 = 0;
    // map RC0 to be the SCLK output
    RC0PPS = 0b11000;

    // MISO/SDI config
    // configure RC1 (pin 9) as digital input
    TRISCbits.TRISC1 = 1;
    // disable analog input
    ANSELCbits.ANSC1 = 0;
    // map RC1 to be MISO
    SSP1DATPPS = 0b10001;

    // MOSI/SDO config
    if (radioType == TRANSMITTER) {
        // configure RC5 (pin 5) as digital output
        TRISCbits.TRISC5 = 0;
        // map RA4 to be MOSI
        RC5PPS = 0b11001;
    } else if (radioType == RECEIVER) {
        // configure RA2 (pin 11) as digital output
        TRISAbits.TRISA2 = 0;
        // map RA4 to be MOSI
        RA2PPS = 0b11001;
    }

    // CS configure
    // configure RA4 (pin 3) as digital output
    TRISAbits.TRISA4 = 0;
    // set chip select line to be default HIGH
    LATAbits.LATA4 = 1;

    // enables serial port and configures SCK, SDO, SDI and SS as the source of the serial PORT pins
    SSP1CON1bits.SSPEN = 1;
    // SPI Master mode, clock = FOSC/4
    SSP1CON1bits.SSPM = 0b0000;
    // input data sampled at middle of data output time
    // this bit needs to be set otherwise SPI doesn't work
    SSP1STATbits.SMP = 1;
    // CLOCK PHASE: transmit occurs on transition from active to Idle clock state
    SSP1STATbits.CKE = 1;
    // CLOCK POLARITY: clock idles low
    SSP1CON1bits.CKP = 0;
}

// configures the nRF24L01 radio as a receiver when run
void configureRX(uint8_t *address) {
    // open reading pipe
    uint8_t bytes[2];
    uint8_t result[2];
    bytes[0] = W_REGISTER | EN_RXADDR;
    bytes[1] = 0x01;
    SendSPI(bytes, result, 2);

    // setup data pipe for receiving data
    StartListening(address);

    // clear status register
    bytes[0] = W_REGISTER | SPI_STATUS;
    bytes[1] = 0x70;
    SendSPI(bytes, result, 2);

    // configure timer 2 for PWM5 module
    T2CONbits.TMR2ON = 0;       // turn off timer 2
    T2CONbits.T2CKPS = 0b01;    // 1:4 pre scaler
    TMR2 = 0;                   // clear timer 2 counter bits
    PR2 = 125;                  // set timer 2 period 125us
    T2CONbits.TMR2ON = 1;       // turn on timer 2

    // configure timer 4 for PWM6 module
    T4CONbits.TMR4ON = 0;       // turn off timer 2
    T4CONbits.T4CKPS = 0b11;    // 1:16 pre scaler
    TMR4 = 0;                   // clear timer 2 counter bits
    PR4 = 77;                  // set timer 2 period 125us
    T4CONbits.TMR4ON = 1;       // turn on timer 2
    
    // PWM5.1 setup
    // configure RC4 (pin 6) as digital output
    TRISCbits.TRISC4 = 0;
    // set RC3 output to be default low
    LATCbits.LATC4 = 0;
    
    // PWM5 setup
    // configure RC2 (pin 8) as digital output
    TRISCbits.TRISC2 = 0;
    // disable PWM5 module
    PWM5CONbits.PWM5EN = 0;
    // PWM5 output is active-high
    PWM5CONbits.PWM5POL = 0;
    // map RC2 to PWM5
    RC2PPS = 0b00010;
    // set duty cycle to 0
    PWM5DCL = 0x00;
    // set duty cycle to 0
    PWM5DCH = 0x00;
    // select timer 2 as timing source
    PWMTMRSbits.P5TSEL = 0x01;
    // enable PWM5 module
    PWM5CONbits.PWM5EN = 1;

    // PWM6 setup
    // configure RC3 (pin 7) as digital output
    TRISCbits.TRISC3 = 0;
    // disable PWM6 module
    PWM6CONbits.PWM6EN = 0;
    // PWM6 output is active-high
    PWM6CONbits.PWM6POL = 0;
    // map RC3 to PWM6
    RC3PPS = 0b00011;
    // set duty cycle to 0
    PWM6DCL = 0xC0;
    // set duty cycle to 0
    PWM6DCH = 0x07;
    // select timer 2 as timing source
    PWMTMRSbits.P6TSEL = 0x02;
    // enable PWM6 module
    PWM6CONbits.PWM6EN = 1;
}

// configures the nRF24L01 as a transmitter when run
void configureTX(uint8_t *address) {
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
    ChangeRadioMode(Standby1, 1, TRANSMITTER);

    // SPEED setup
    // configure RC4 (pin 6) as digital input
    TRISCbits.TRISC4 = 1;
    // enable analog input
    ANSELCbits.ANSC4 = 1;
    
    // STEER setup
    // configure RC3 (pin 7) as digital input
    TRISCbits.TRISC3 = 1;
    // enable analog input
    ANSELCbits.ANSC3 = 1;
    
    // STEER_TRIM setup
    // configure RA1 (pin 12) as digital input
    TRISAbits.TRISA1 = 1;
    // enable analog input
    ANSELAbits.ANSA1 = 1;

    ADCON0bits.ADON = 0;        // turn off ADC
    ADCON1bits.ADCS = 0b110;    // select ADC conversion clock as Fosc/2
    ADCON1bits.ADNREF = 0;      // set negative voltage reference as Vss
    ADCON1bits.ADPREF = 0;      // set positive voltage reference as Vdd
    ADCON0bits.CHS = 0x14;      // select analog input channel as RC4
    ADCON1bits.ADFM = 1;        // result is stored right justified
    ADCON0bits.ADON = 1;        // turn on ADC
}

// functions checks if the control inputs have changed. used so radio is not 
// retransmitting the same message or the controls resting values
bool controlsChanged(uint8_t *bytes) {
    // holds previous control values
    static uint8_t prevBytes[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t delta = 20;
    bool returnVal = false;
    // checks if any of the high or low bytes of the ADC conversions are different
    for (uint8_t i = 0; i < 6; i++) {
        if (((prevBytes[i] - bytes[i]) > delta) || ((bytes[i] - prevBytes[i]) > delta)) {
            returnVal = true;
            prevBytes[i] = bytes[i];
        }
    }
    return returnVal;
}