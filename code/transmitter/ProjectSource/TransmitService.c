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
#define AN0_AD_CHANNEL		(1 << 0)	// SPEED
#define AN1_AD_CHANNEL 		(1 << 1)	// SPEED_TRIM
#define AN2_AD_CHANNEL		(1 << 2)	// STEER_TRIM
#define AN3_AD_CHANNEL		(1 << 3)	// STEER

/*---------------------------- Module Functions ---------------------------*/

/*---------------------------- Module Variables ---------------------------*/
static uint8_t MyPriority;
uint8_t address[] = {0x30, 0x30, 0x30, 0x31, 0x31};
uint8_t payload[PAYLOAD_SIZE + 1];
STATUSbits_t STATUSbits;
static uint8_t motorSpeed = 128;
static uint8_t servoPos = 0;
bool readyToTransmit = true;

/*------------------------------ Module Code ------------------------------*/
bool InitTransmitService(uint8_t Priority) {
	clrScrn();
	DB_printf("Init Transmit Service\n");
	ES_Event_t ThisEvent;
	MyPriority = Priority;

	// LED - pin 6 - RB2 - input
	TRISBbits.TRISB2 = 0;
	LATBbits.LATB2 = 0;

	// CE - pin 26 - RB15 - input
	TRISBbits.TRISB15 = 0;
	ce(LOW);

	// SPI config
	if (true) {
		DB_printf("Configuring SPI bus\n");
		SPI1CONbits.ON = 0;         // disable SPI1
		SPI1CONbits.MCLKSEL = 0;    // Configure the SPI clock to be based on PBCLK
		SPI1CONbits.FRMEN = 0;      // Disable the Framed mode
		SPI1CON2bits.AUDEN = 0;     // Disable the Audio mode

		SPI1CONbits.MSTEN = 1;      // select master mode
		SPI1CONbits.SMP = 0;        // input data sampled at middle of data output time

		// SCLK - pin 25 - RB14 - input
		TRISBbits.TRISB14 = 0;

		// MISO/SDI - pin 22 - RB11 - input
		TRISBbits.TRISB11 = 1;
		SDI1R = 0b0011;             // map SDI1 to RB11

		// MOSI/SDO - pin 24 - RB13 - output
		TRISBbits.TRISB13 = 0;
		RPB13R = 0b0011;            // map RB13 to SDO1

		// CSN - pin 23 - RB12 - output
		TRISBbits.TRISB12 = 0;
		// set high as default
		LATBbits.LATB12 = 1;
		// RPB15R = 0b0011;

		// SPI1BRG = 2000;  				// set baud rate SPI1BRG = 2 = 20MHz / (2 * 8MHz - 1)
		SPI1BRG = 2;
		SPI1CONbits.ENHBUF = 0;     // disable enhanced buffer
		SPI1STATbits.SPIROV = 0;    // receive overflow flag bit

		SPI1CONbits.CKE = 1;        // data transmit on idle to active
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
		// IRQ - pin 21 - RB10 - input
		TRISBbits.TRISB10 = 1;
		// map INT1 to RB10
		INT1R = 0b0011;
		// enable multi-vector mode
		INTCONbits.MVEC = 1;
		// interrupt on falling edge
		INTCONbits.INT1EP = 0;
		// set interrupt priority
		IPC1bits.INT1IP = 7;
		// set interrupt sub priority
		IPC1bits.INT1IS = 0;
		// clear INT1 interrupt flag
		IFS0CLR = _IFS0_INT1IF_MASK;
		// enable INT1 interrupt
		IEC0SET = _IEC0_INT1IE_MASK;
		// enable all interrupts
		__builtin_enable_interrupts();
	}
	
	// start radio
	bool RadioStarted = false;
	uint8_t result[2];
	if (StartRadio(CHANNEL, PAYLOAD_SIZE, result)) {
		DB_printf("Successfully communicated with radio.\n");
		RadioStarted = true;
	} else {
		DB_printf("Unsuccessfully communicated with radio.\n");
	}
	STATUSbits.w = result[0];

	// setup radio as transmitter
	if (RadioStarted) {
		// set address
		uint8_t result[2];
		WriteRegister(RX_ADDR_P0, address, result, ADDRESS_WIDTH);
		WriteRegister(TX_ADDR, address, result, ADDRESS_WIDTH);
		STATUSbits.w = result[0];

		// power up radio
		ChangeRadioMode(Standby1, 1, result);
		STATUSbits.w = result[0];
		DB_printf("Radio configured as a transmitter\n");

		// SPEED - pin 2 - RA0 - input
		TRISAbits.TRISA0 = 1;
		// enable analog input on pin 2 AN0
		ANSELAbits.ANSA0 = 1;

		// SPEED_TRIM - pin 3 - RA1 - input
		TRISAbits.TRISA1 = 1;
		// enable analog input on pin 3 AN1
		ANSELAbits.ANSA1 = 1;

		// STEER_TRIM - pin 9 - RA2 - input
		// TRISAbits.TRISA2 = 1;
		// enable analog input on pin 9
		// ANSELAbits.ANSA2 = 1;

		// STEER - pin 10 - RA3 - input
		// TRISAbits.TRISA3 = 1;
		// enable analog input on pin 10
		// ANSELAbits.ANSA3 = 1;

		ADC_ConfigAutoScan(AN0_AD_CHANNEL | AN1_AD_CHANNEL);
		// uint32_t ADValues[2];
		// ADC_MultiRead(ADValues);
		// DB_printf("ADValues[0] = %d\n", ADValues[0]);
		// DB_printf("ADValues[1] = %d\n", ADValues[1]);
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
			// PackagePayload(motorSpeed, servoPos);
			// TransmitPayload();
			// ES_Timer_InitTimer(SERVICE_TIMER, 1000);
			break;
		}

	case ES_STATUS_FLAGS:
		{
			DB_printf("STATUSbits = 0x%x\n", STATUSbits);
			if (STATUSbits.RX_DR) {
				// DB_printf("Data ready in RX FIFO\n");
				DB_printf("SHOULD NOT BE HERE!!!\n");
			} else if (STATUSbits.TX_DS) {
				DB_printf("Data successfully sent from TX FIFO\n");
				readyToTransmit = true;
			} else if (STATUSbits.MAX_RT) {
				DB_printf("Max number of TX transmits reached\n");
				readyToTransmit = false;
				uint8_t databytes[] = {0x70};
				uint8_t result[2];
				WriteRegister(STATUS, databytes, result, 1);
				DB_printf("result[0] = 0x%x\n", result[0]);
				STATUSbits.w = result[0];
			} else {
				DB_printf("SHOULD NOT BE HERE!!!\n");
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

	case ES_NEW_KEY:
		{
			DB_printf("ES_NEW_KEY: %d\n", ThisEvent.EventParam);
			PackagePayload((uint8_t)ThisEvent.EventParam, (uint8_t)ThisEvent.EventParam);
			// for (uint8_t i = 0; i < 5; i++) {
			// 	DB_printf("payload[%d] = %d\n", i, payload[i]);
			// }
			FlushTX();
			TransmitPayload();
			break;
		}

	case ES_TIMEOUT:
		{
			// DB_printf("ES_TIMEOUT\n");
			uint8_t databytes[] = {NOP};
			ReadRegister(STATUS, databytes);
			ES_Timer_InitTimer(SERVICE_TIMER, 1000);
			break;
		}
	}
	return ReturnEvent;
}

/***************************************************************************
private functions
***************************************************************************/
void __ISR(_EXTERNAL_1_VECTOR, IPL7SOFT) INT1Handler(void) {
	IFS0CLR = _IFS0_INT1IF_MASK;
	uint8_t databytes[] = {0x70};
	uint8_t result[2];
	WriteRegister(STATUS, databytes, result, 1);
	STATUSbits.w = result[0];
	ES_Event_t ThisEvent;
	ThisEvent.EventType = ES_STATUS_FLAGS;
	PostTransmitService(ThisEvent);
}

void ce(Level_t Level) {
	if (Level == LOW) {
		LATBbits.LATB15 = 0;
	} else if (Level == HIGH) {
		LATBbits.LATB15 = 1;
	}
}

void TransmitPayload(void) {
	readyToTransmit = false;
	// DB_printf("Writing payload to radio\n");
	if (STATUSbits.w & 0x70 || true) {
		// clear STATUS register to allow for more transmissions
		uint8_t databytes[] = {0x70};
		uint8_t result[2];
		WriteRegister(STATUS, databytes, result, 1);
		STATUSbits.w = result[0];
		DB_printf("STATUSbits = 0x%x\n", STATUSbits);
	}
	uint8_t result[PAYLOAD_SIZE + 1];
	SendSPI(payload, result, PAYLOAD_SIZE + 1);
	ce(HIGH);
	delay(1000);
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
