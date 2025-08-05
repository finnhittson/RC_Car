/*----------------------------- Include Files -----------------------------*/
#include <xc.h>
#include <sys/attribs.h>
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "TransmitService.h"
#include "dbprintf.h"
#include "PIC32_AD_Lib.h"

/*----------------------------- Module Defines ----------------------------*/
// radio specific defined constants
#define ADDRESS_WIDTH		5
#define PAYLOAD_SIZE		4
#define CHANNEL				42
#define RADIO_ID			0x01
#define MOTOR_PERIOD		50000
#define AD_CHANNEL1			(1 << 0)
#define AD_CHANNEL2			(1 << 1)

/*---------------------------- Module Functions ---------------------------*/

/*---------------------------- Module Variables ---------------------------*/
static uint8_t MyPriority;
uint8_t address[] = {0x30, 0x30, 0x30, 0x31, 0x31};
uint8_t payload[PAYLOAD_SIZE + 1];
STATUSbits_t STATUSbits;
Radio_t radioType = RECEIVER;
static uint8_t motorSpeed = 128;
static uint8_t servoPos = 0;
bool readyToTransmit = true;

/*------------------------------ Module Code ------------------------------*/
bool InitTransmitService(uint8_t Priority) {
	clrScrn();
	DB_printf("Init Transmit/Receive Service\n");
	ES_Event_t ThisEvent;
	MyPriority = Priority;

	// LED config (pin 14)
	TRISBbits.TRISB5 = 0;
	LATBbits.LATB5 = 1;

	// radio CE config (pin 12)
	TRISAbits.TRISA4 = 0;
	ce(LOW);

	// receiver/transmiter selection pin
	TRISBbits.TRISB9 = 1;
	if (PORTBbits.RB9) {
		radioType = TRANSMITTER;
	}

	// SPI config
	if (true) {
		DB_printf("Configuring SPI bus\n");
		SPI1CONbits.ON = 0;         // disable SPI1
		SPI1CONbits.MCLKSEL = 0;    // Configure the SPI clock to be based on PBCLK
		SPI1CONbits.FRMEN = 0;      // Disable the Framed mode
		SPI1CON2bits.AUDEN = 0;     // Disable the Audio mode

		SPI1CONbits.MSTEN = 1;      // select master mode
		SPI1CONbits.SMP = 0;        // input data sampled at middle of data output time

		// SCLK config (RB14 - pin 25)
		TRISBbits.TRISB14 = 0;      // configure as digital input

		// MISO/SDI config (RB11 - pin 22)
		TRISBbits.TRISB11 = 1;      // set MISO as digital input
		SDI1R = 0b0011;             // map SDI1 to RB11

		// MOSI/SDO config (RB13 - pin 24)
		TRISBbits.TRISB13 = 0;      // set MOSI as digital output
		RPB13R = 0b0011;            // map RB13 to SDO1

		// CS config (RB15 - pin 26)
		TRISBbits.TRISB15 = 0;		// set CS as digital output
		LATBbits.LATB15 = 1;
		// RPB15R = 0b0011;

		// SPI1BRG = 2000;  				// set baud rate SPI1BRG = 2 = 20MHz / (2 * 8MHz - 1)
		SPI1BRG = 2;
		SPI1CONbits.ENHBUF = 0;     // disable enhanced buffer
		SPI1STATbits.SPIROV = 0;    // receive overflow flag bit

		SPI1CONbits.CKE = 1;        // data transmit on active to idle
		SPI1CONbits.CKP = 0;        // clock idles low

		SPI1CONbits.MODE32 = 0;     // data transfer 8 bit
		SPI1CONbits.MODE16 = 0;     // data transfer 8 bit

		SPI1CONbits.FRMPOL = 0;     // CS active low
		SPI1CONbits.MSSEN = 0;		// CS line controlled manually

		SPI1CONbits.ON = 1;         // enable SPI1
		SPI1BUF;					// clear SPI1BUF
	}

	// enable interrupts
	if (true) {
		TRISBbits.TRISB8 = 1;			// enable interrput pin as input
		INT3R = 0b0100;					// map pin 17 (RPB8) to input interrupt
		INTCONbits.MVEC = 1;			// enable multi-vector mode
		INTCONbits.INT3EP = 0;			// interrupt on falling edge
		IPC3bits.INT3IP = 7;			// set interrupt priority
		IPC3bits.INT3IS = 0;			// set interrupt sub priority
		IFS0CLR = _IFS0_INT3IF_MASK;	// clear int3 interrupt flag
		IEC0SET = _IEC0_INT3IE_MASK;	// enable int3 interrupt
		__builtin_enable_interrupts();
	}
	
	// start radio
	bool RadioStarted = false;
	uint8_t result[2];
	if (!StartRadio(CHANNEL, PAYLOAD_SIZE, result)) {
		DB_printf("Unsuccessfully communicated with radio.\n");
	} else {
		DB_printf("Successfully communicated with radio.\n");
		RadioStarted = true;
	}
	STATUSbits.w = result[0];

	// setup radio as receiver
	if (RadioStarted && radioType == RECEIVER) {
		// open reading pipe
		uint8_t databytes[] = {0x01};
		uint8_t result[2];
		WriteRegister(EN_RXADDR, databytes, result, 1);
		STATUSbits.w = result[0];

		// setup datapipe for receiving data
		StartListening(address, ADDRESS_WIDTH, result);
		STATUSbits.w = result[0];

		// clear status register
		databytes[0] = 0x70;
		WriteRegister(STATUS, databytes, result, 1);
		STATUSbits.w = result[0];
		DB_printf("Radio configured as a receiver\n");

		// init timer2
		T2CONbits.ON = 0;			// turn off timer2
		T2CONbits.TCS = 0;			// set timer2 source
		T2CONbits.TGATE = 0;		// disable TGATE
		T2CONbits.TCKPS = 1;		// set timer2 pre-scaler 1:8
		PR2 = MOTOR_PERIOD - 1;		// set timer2 period
		TMR2 = 0;					// set timer2 to start at 0
		IFS0CLR = _IFS0_T2IF_MASK;	// clear timer2 interrupt flags
		T2CONbits.ON = 1;			// enable timer2

		// PWM forwards - output compare module 1
		TRISAbits.TRISA0 = 0;		// set pin 2 (RA0) as output
		ANSELAbits.ANSA0 = 0;		// set pin 2 as digital
		OC1CONbits.ON = 0;			// disable module
		OC1CONbits.SIDL = 0;		// set to continue in idle mode
		RPA0R = 0b0101;				// map pin 2 to OC module
		OC1CONbits.OC32 = 0;		// use 16 bit timer source
		OC1CONbits.OCTSEL = 0;		// use timer2
		OC1R = 0;					// set duty cycle
		OC1RS = 0;					// set duty cycle
		OC1CONbits.OCM = 6;			// set as pwm
		OC1CONbits.ON = 1;			// enable output compare module 1

		// PWM backwards - output compare module 2
		TRISAbits.TRISA1 = 0;		// set pin 3 (RA1) as output
		ANSELAbits.ANSA1 = 0;		// set pin 3 as digital
		OC2CONbits.ON = 0;			// disable module
		OC2CONbits.SIDL = 0;		// set to continue in idle mode
		RPA1R = 0b0101;				// map pin 3 to OC module
		OC2CONbits.OC32 = 0;		// use 16 bit timer source
		OC2CONbits.OCTSEL = 0;		// use timer2
		OC2R = 0;					// set duty cycle
		OC2RS = 0;					// set duty cycle
		OC2CONbits.OCM = 6;			// set as pwm
		OC2CONbits.ON = 1;			// enable output compare module 2

		// servo PWM - output compare module 3
		TRISAbits.TRISA3 = 0;		// set pin 3 (RA1) as output
		OC3CONbits.ON = 0;			// disable module
		OC3CONbits.SIDL = 0;		// set to continue in idle mode
		RPA3R = 0b0101;				// map pin 3 to OC module
		OC3CONbits.OC32 = 0;		// use 16 bit timer source
		OC3CONbits.OCTSEL = 0;		// use timer2
		OC3R = 3750;				// set duty cycle
		OC3RS = 3750;				// set duty cycle
		OC3CONbits.OCM = 6;			// set as pwm
		OC3CONbits.ON = 1;			// enable output compare module 2
	}

	// setup radio as transmitter
	else if (RadioStarted && radioType == TRANSMITTER) {
		// set address
		uint8_t result[2];
		WriteRegister(RX_ADDR_P0, address, result, ADDRESS_WIDTH);
		WriteRegister(TX_ADDR, address, result, ADDRESS_WIDTH);
		STATUSbits.w = result[0];

		// power up radio
		ChangeRadioMode(Standby1, 1, result);
		STATUSbits.w = result[0];
		DB_printf("Radio configured as a transmitter\n");

		ANSELAbits.ANSA0 = 1;
		TRISAbits.TRISA0 = 1;

		ANSELAbits.ANSA1 = 1;
		TRISAbits.TRISA1 = 1;

		ADC_ConfigAutoScan(AD_CHANNEL1 | AD_CHANNEL2);
	}

	ThisEvent.EventType = ES_INIT;
	if (ES_PostToService(MyPriority, ThisEvent) == true) {
		return true;
	} else {
		return false;
	}
}

bool PostTransmitService(ES_Event_t ThisEvent) {
	return ES_PostToService(MyPriority, ThisEvent);
}

ES_Event_t RunTransmitService(ES_Event_t ThisEvent) {
	ES_Event_t ReturnEvent;
	ReturnEvent.EventType = ES_NO_EVENT;
	switch (ThisEvent.EventType) {
	case ES_INIT:
		{
			if (radioType == TRANSMITTER && false) {
				PackagePayload(motorSpeed, servoPos);
				TransmitPayload();
			}
			// ES_Timer_InitTimer(SERVICE_TIMER, 1000);
			break;
		}

	case ES_STATUS_FLAGS:
		{
			if (STATUSbits.RX_DR) {
				DB_printf("Data ready in RX FIFO\n");
				if (!radioIsTransmitter()) {
					ThisEvent.EventType = ES_HANDLE_PAYLOAD;
					PostTransmitService(ThisEvent);
				}
			} else if (STATUSbits.TX_DS) {
				DB_printf("Data successfully sent from TX FIFO\n");
				readyToTransmit = true;
			} else if (STATUSbits.MAX_RT) {
				DB_printf("Max number of TX transmits reached\n");
			} else {
				DB_printf("SHOULD NOT BE HERE!!!\n");
			}
			break;
		}

	case ES_HANDLE_PAYLOAD:
		{
			uint8_t result[PAYLOAD_SIZE + 1];
			bool validData = ReadRXFIFO(PAYLOAD_SIZE, result);
			DB_printf("HERE\n");
			if (result[1] == RADIO_ID) { // && validData
				uint8_t newMotorSpeed = result[2];
				uint8_t newServoPos = result[3];
				DB_printf("Updating motor speed: %d\n", newMotorSpeed);
				// update motor speed
				if (newMotorSpeed < 128) {
					OC1RS = 0;
					OC2RS = MOTOR_PERIOD - ((newMotorSpeed * MOTOR_PERIOD) / 128);
					DB_printf("motor speed: %d\n", MOTOR_PERIOD - ((newMotorSpeed * MOTOR_PERIOD) / 128));
				} else {
					OC2RS = 0;
					OC1RS = ((newMotorSpeed * MOTOR_PERIOD) / 128) - MOTOR_PERIOD;
					DB_printf("motor speed: %d\n", ((newMotorSpeed * MOTOR_PERIOD) / 128) - MOTOR_PERIOD);
				}
				DB_printf("Updating servo position: %d\n", newServoPos);
				// update servo position
				OC3RS = 23.4375 * newServoPos + 2250;
			} else if (result[1] != RADIO_ID) {
				DB_printf("Radio ID does not match: %d\n", RADIO_ID);
			} else if (!validData) {
				DB_printf("Invalid data recieved. Undesierable checksum value.\n");
			}
			break;
		}

	case ES_CONTROL_UPDATE:
		{
			motorSpeed = ThisEvent.EventParam >> 8;
			servoPos = ThisEvent.EventParam;
			PackagePayload(motorSpeed, servoPos);
			if (readyToTransmit) {
				DB_printf("Update motor speed: %d\n", motorSpeed);
				DB_printf("Update servo pos: %d\n\n", servoPos);
				TransmitPayload();
			}
			break;
		}

	case ES_TIMEOUT:
		{
			uint8_t databytes[] = {NOP};
			ReadRegister(STATUS, databytes);
			ES_Timer_InitTimer(SERVICE_TIMER, 1000);
		}
	}
	return ReturnEvent;
}

/***************************************************************************
private functions
***************************************************************************/
void __ISR(_EXTERNAL_3_VECTOR, IPL7SOFT) INT3Handler(void) {
	IFS0CLR = _IFS0_INT3IF_MASK;
	// DB_printf("Interrupt occured\n");
	uint8_t databytes[] = {0x70};
	uint8_t result[2];
	WriteRegister(STATUS, databytes, result, 1);
	STATUSbits.w = result[0];
	ES_Event_t ThisEvent;
	ThisEvent.EventType = ES_STATUS_FLAGS;
	PostTransmitService(ThisEvent);
}

bool radioIsTransmitter(void) {
	if (radioType == TRANSMITTER) {
		return true;
	}
	return false;
}

void ce(Level_t Level) {
	if (Level == LOW) {
		LATAbits.LATA4 = 0;
	} else if (Level == HIGH) {
		LATAbits.LATA4 = 1;
	}
}

void TransmitPayload(void) {
	readyToTransmit = false;
	// DB_printf("Writing payload to radio\n");
	if (STATUSbits.w & 0x70) {
		// clear STATUS register to allow for more transmissions
		uint8_t databytes[] = {0x70};
		uint8_t result[2];
		WriteRegister(STATUS, databytes, result, 1);
		STATUSbits.w = result[0];
	}
	uint8_t result[PAYLOAD_SIZE + 1];
	SendSPI(payload, result, PAYLOAD_SIZE + 1);
	ce(HIGH);
	delay(TX_DELAY);
	ce(LOW);
}

void PackagePayload(uint8_t data1, uint8_t data2) {
	uint8_t checksum = 0xFF - (RADIO_ID + data1 + data2);
	payload[0] = W_TX_PAYLOAD;
	payload[1] = RADIO_ID;
	payload[2] = data1;
	payload[3] = data2;
	payload[4] = checksum;
}
