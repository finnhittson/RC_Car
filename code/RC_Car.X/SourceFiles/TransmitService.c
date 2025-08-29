/*----------------------------- Include Files -----------------------------*/
#include <xc.h>
//#include <sys/attribs.h>
//#include "ES_Configure.h"
//#include "ES_Framework.h"
#include "../HeaderFiles/TransmitService.h"
//#include "dbprintf.h"
//#include "PIC32_AD_Lib.h"
#include "stdbool.h"

/*----------------------------- Module Defines ----------------------------*/
// radio specific defined constants
#define ADDRESS_WIDTH		5

/*---------------------------- Module Functions ---------------------------*/

/*---------------------------- Module Variables ---------------------------*/
uint8_t payload[PAYLOAD_SIZE + 1];
Radio_t radioType = RECEIVER;
static uint8_t motorSpeed = 128;
static uint8_t servoPos = 0;
bool readyToTransmit = true;

/*------------------------------ Module Code ------------------------------*/
Radio_t InitTransmitService() {
//	clrScrn();
//	DB_printf("Init Transmit/Receive Service\n");
	ES_Event_t ThisEvent;
	
	uint8_t result[2];
//	SPI_STATUSbits.w = result[0];
    bool RadioStarted = false;
	// setup radio as receiver
	

	

	return radioType;
}

ES_Event_t RunTransmitService(ES_Event_t ThisEvent) {
	ES_Event_t ReturnEvent;
	switch (ThisEvent.EventType) {
	case ES_INIT:
		{
//			if (radioType == TRANSMITTER && false) {
//				PackagePayload(motorSpeed, servoPos);
//				TransmitPayload();
//			}
//			// ES_Timer_InitTimer(SERVICE_TIMER, 1000);
			break;
		}

	case ES_STATUS_FLAGS:
		{
//			if (SPI_STATUSbits.RX_DR) {
////				DB_printf("Data ready in RX FIFO\n");
//				if (!radioIsTransmitter()) {
//					ThisEvent.EventType = ES_HANDLE_PAYLOAD;
////					PostTransmitService(ThisEvent);
//				}
//			} else if (SPI_STATUSbits.TX_DS) {
////				DB_printf("Data successfully sent from TX FIFO\n");
//				readyToTransmit = true;
//			} else if (SPI_STATUSbits.MAX_RT) {
////				DB_printf("Max number of TX transmits reached\n");
//			} else {
////				DB_printf("SHOULD NOT BE HERE!!!\n");
//			}
			break;
		}

	case ES_HANDLE_PAYLOAD:
		{
			uint8_t result[PAYLOAD_SIZE + 1];
			bool validData = readRXFIFO(result);
//			DB_printf("HERE\n");
			if (result[1] == 0) { // && validData
				uint8_t newMotorSpeed = result[2];
				uint8_t newServoPos = result[3];
//				DB_printf("Updating motor speed: %d\n", newMotorSpeed);
				// update motor speed
				if (newMotorSpeed < 128) {
//					OC1RS = 0;
//					OC2RS = MOTOR_PERIOD - ((newMotorSpeed * MOTOR_PERIOD) / 128);
//					DB_printf("motor speed: %d\n", MOTOR_PERIOD - ((newMotorSpeed * MOTOR_PERIOD) / 128));
				} else {
//					OC2RS = 0;
//					OC1RS = ((newMotorSpeed * MOTOR_PERIOD) / 128) - MOTOR_PERIOD;
//					DB_printf("motor speed: %d\n", ((newMotorSpeed * MOTOR_PERIOD) / 128) - MOTOR_PERIOD);
				}
//				DB_printf("Updating servo position: %d\n", newServoPos);
				// update servo position
//				OC3RS = 23.4375 * newServoPos + 2250;
			} else if (result[1] != 0) {
//				DB_printf("Radio ID does not match: %d\n", RADIO_ID);
			} else if (!validData) {
//				DB_printf("Invalid data recieved. Undesierable checksum value.\n");
			}
			break;
		}

	case ES_CONTROL_UPDATE:
		{
			motorSpeed = ThisEvent.EventParam >> 8;
			servoPos = (uint8_t)ThisEvent.EventParam;
//			PackagePayload(motorSpeed, servoPos);
			if (readyToTransmit) {
//				DB_printf("Update motor speed: %d\n", motorSpeed);
//				DB_printf("Update servo pos: %d\n\n", servoPos);
//				TransmitPayload();
			}
			break;
		}
    
    default:
        {
            break;
        }
	}
	return ReturnEvent;
}

/***************************************************************************
private functions
***************************************************************************/

void transmitPayload(uint8_t statusBits) {
	readyToTransmit = false;
	// DB_printf("Writing payload to radio\n");
	if (statusBits & 0x70) {
		// clear STATUS register to allow for more transmissions
		uint8_t databytes[2];
        uint8_t result[2];
        databytes[0] = W_REGISTER | SPI_STATUS;
        databytes[1] = 0x70;
        SendSPI(databytes, result, 2);
	}
	uint8_t result[PAYLOAD_SIZE + 1];
	SendSPI(payload, result, PAYLOAD_SIZE + 1);
	ce(HIGH);
	delay(TX_DELAY);
	ce(LOW);
}

void packagePayload(uint8_t radioID, uint8_t bytes1, uint8_t bytes2, uint8_t bytes3, uint8_t bytes4) {
	uint8_t checksum = 0xFF - (radioID + bytes1 + bytes2 + bytes3 + bytes4);
	payload[0] = W_TX_PAYLOAD;
	payload[1] = radioID;
	payload[2] = bytes1;
	payload[3] = bytes2;
	payload[4] = bytes3;
    payload[5] = bytes4;
    payload[6] = checksum;
}

void setupSPI(void) {
    // SCLK config
    TRISCbits.TRISC0 = 0;
    // map RC0 to be the SCLK output
    RC0PPS = 0b11000;

    // MISO/SDI config
    TRISCbits.TRISC1 = 1;
    // disable analog input
    ANSELCbits.ANSC1 = 0;
    // map RC1 to be MISO
    SSP1DATPPS = 0b10001;

    // MOSI/SDO config
    TRISAbits.TRISA4 = 0;
    // map RA4 to be MOSI
    RA4PPS = 0b11001;

    // CS configure
    TRISCbits.TRISC3 = 0;
    // set chip select line to be default HIGH
    LATCbits.LATC3 = 1;

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

    // configure timer 2 for PWM channels
    T2CONbits.TMR2ON = 0;       // turn off timer 2
    T2CONbits.T2CKPS = 0b01;    // 1:4 pre scaler
    TMR2 = 0;                   // clear timer 2 counter bits
    PR2 = 125;                  // set timer 2 period 125us
    T2CONbits.TMR2ON = 1;       // turn on timer 2

    TRISCbits.TRISC4 = 0;
    LATCbits.LATC4 = 0;
    
    // configure PMW5 module
    TRISCbits.TRISC2 = 0;       // set RC2 as output
    PWM5CONbits.PWM5EN = 0;     // disable PWM5 module
    PWM5CONbits.PWM5POL = 0;    // PWM5 output is active-high
    RC2PPS = 0b00010;           // map RC2 to PWM5
    PWM5DCL = 0xC0;             // set duty cycle to 10
    PWM5DCH = 0x6F;             // set duty cycle to 10
    PWMTMRSbits.P5TSEL = 0b01;  // select timer 2 as timing source
    PWM5CONbits.PWM5EN = 1;     // enable PWM5 module

    // configure PWM6 module
    TRISCbits.TRISC4 = 0;       // set RC4 as output
    PWM6CONbits.PWM6EN = 0;     // disable PWM5 module
    PWM6CONbits.PWM6POL = 0;    // PWM5 output is active-high
    RC4PPS = 0b00011;           // map RA4 to PWM6
    PWM6DCL = 0x00;             // set duty cycle to 10
    PWM6DCH = 0x00;             // set duty cycle to 10
    PWMTMRSbits.P6TSEL = 0b01;  // select timer 2 as timing source
    PWM6CONbits.PWM6EN = 1;     // enable PWM5 module
}

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
    ChangeRadioMode(Standby1, 1);

    // configure analog input
    ANSELCbits.ANSC4 = 1;
    TRISCbits.TRISC4 = 1;

    ADCON0bits.ADON = 0;        // turn off ADC
    ADCON1bits.ADCS = 0b110;    // select ADC conversion clock as Fosc/2
    ADCON1bits.ADNREF = 0;      // set negative voltage reference as Vss
    ADCON1bits.ADPREF = 0;      // set positive voltage reference as Vdd
    ADCON0bits.CHS = 0x14;      // select analog input channel as RC4
    ADCON1bits.ADFM = 1;        // result is stored right justified
    ADCON0bits.ADON = 1;        // turn on ADC
}