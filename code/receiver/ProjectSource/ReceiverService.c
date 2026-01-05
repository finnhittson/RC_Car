/*----------------------------- Include Files -----------------------------*/
#include <xc.h>
#include <sys/attribs.h>
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ReceiverService.h"
#include "dbprintf.h"
#include "PIC32_AD_Lib.h"

/*----------------------------- Module Defines ----------------------------*/
// radio specific defined constants
#define ADDRESS_WIDTH		5
#define PAYLOAD_SIZE		4
#define CHANNEL				42
#define RADIO_ID			0x01
#define MOTOR_PERIOD		50000
#define BLINK_TIME			50

/*---------------------------- Module Functions ---------------------------*/

/*---------------------------- Module Variables ---------------------------*/
static uint8_t MyPriority;
uint8_t address[] = {0x30, 0x30, 0x30, 0x31, 0x31};
uint8_t payload[PAYLOAD_SIZE + 1];
STATUSbits_t STATUSbits;
static uint8_t motorSpeed = 128;
static uint8_t servoPos = 0;
bool readyToTransmit = true;
uint8_t blinkCount = 0;
volatile bool LEDStatus = false;

/*------------------------------ Module Code ------------------------------*/
bool InitReceiverService(uint8_t Priority) {
	clrScrn();
	DB_printf("Init Receive Service\n");
	ES_Event_t ThisEvent;
	MyPriority = Priority;

	// LED - pin 9 - RA2 - input
	TRISAbits.TRISA2 = 0;
	LATAbits.LATA2 = 0;

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

		// SCK - pin 25 - RB14 - input
		TRISBbits.TRISB14 = 0;

		// MISO - pin 22 - RB11 - input
		TRISBbits.TRISB11 = 1;
		// map SDI1 to RB11
		SDI1R = 0b0011;

		// MOSI - pin 24 - RB13 - output
		TRISBbits.TRISB13 = 0;
		// map RB13 to SDO1
		RPB13R = 0b0011;

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

	// setup radio as receiver
	if (RadioStarted) {
		// open reading pipe
		uint8_t databytes[] = {0x01};
		// uint8_t result[2];
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
		TRISBbits.TRISB4 = 0;		// PWM4 - pin 11 - RB4 - output
		OC1CONbits.ON = 0;			// disable module
		OC1CONbits.SIDL = 0;		// set to continue in idle mode
		RPB4R = 0b0101;				// map pin 11 to OC1 module
		OC1CONbits.OC32 = 0;		// use 16 bit timer source
		OC1CONbits.OCTSEL = 0;		// use timer2
		OC1R = 0;					// set duty cycle
		OC1RS = 0;					// set duty cycle
		OC1CONbits.OCM = 6;			// set as pwm
		OC1CONbits.ON = 1;			// enable output compare module 1

		// PWM backwards - output compare module 2
		TRISBbits.TRISB5 = 0;		// PWM5 - pin 14 - RB5 - output
		OC2CONbits.ON = 0;			// disable module
		OC2CONbits.SIDL = 0;		// set to continue in idle mode
		RPB5R = 0b0101;				// map pin 14 to OC2 module
		OC2CONbits.OC32 = 0;		// use 16 bit timer source
		OC2CONbits.OCTSEL = 0;		// use timer2
		OC2R = 0;					// set duty cycle
		OC2RS = 0;					// set duty cycle
		OC2CONbits.OCM = 6;			// set as pwm
		OC2CONbits.ON = 1;			// enable output compare module 2

		// servo PWM - output compare module 4
		TRISAbits.TRISA3 = 0;		// PWM6 - pin 12 - RA4 - output
		OC4CONbits.ON = 0;			// disable module
		OC4CONbits.SIDL = 0;		// set to continue in idle mode
		RPA4R = 0b0101;				// map pin 12 to OC4 module
		OC4CONbits.OC32 = 0;		// use 16 bit timer source
		OC4CONbits.OCTSEL = 0;		// use timer2
		OC4R = 3750;				// set duty cycle
		OC4RS = 3750;				// set duty cycle
		OC4CONbits.OCM = 6;			// set as pwm
		OC4CONbits.ON = 1;			// enable output compare module 2
	}

	ThisEvent.EventType = ES_INIT;
	if (ES_PostToService(MyPriority, ThisEvent) == true) {
		return true;
	} else {
		return false;
	}
}

bool PostReceiverService(ES_Event_t ThisEvent) {
	return ES_PostToService(MyPriority, ThisEvent);
}

ES_Event_t RunReceiverService(ES_Event_t ThisEvent) {
	ES_Event_t ReturnEvent;
	ReturnEvent.EventType = ES_NO_EVENT;
	switch (ThisEvent.EventType) {
	case ES_INIT:
		{
			break;
		}

	case ES_STATUS_FLAGS:
		{
			if (STATUSbits.RX_DR) {
				DB_printf("Data ready in RX FIFO\n");
				ThisEvent.EventType = ES_BLINK;
				PostReceiverService(ThisEvent);
				// ThisEvent.EventType = ES_HANDLE_PAYLOAD;
				// PostReceiverService(ThisEvent);
			} else if (STATUSbits.TX_DS) {
				DB_printf("Data successfully sent from TX FIFO\n");
				readyToTransmit = true;
			} else if (STATUSbits.MAX_RT) {
				DB_printf("Max number of TX transmits reached\n");
				readyToTransmit = false;
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
				OC4RS = 23.4375 * newServoPos + 2250;
			} else if (result[1] != RADIO_ID) {
				DB_printf("Radio ID does not match: %d\n", RADIO_ID);
			} else if (!validData) {
				DB_printf("Invalid data recieved. Undesierable checksum value.\n");
			}
			break;
		}

	case ES_BLINK:
		{
			LATAbits.LATA2 = !LATAbits.LATA2;
			blinkCount++;
			ES_Timer_InitTimer(SERVICE_TIMER, BLINK_TIME);			
			break;
		}

	case ES_TIMEOUT:
		{
			if (blinkCount < 20) {
				ThisEvent.EventType = ES_BLINK;
				PostReceiverService(ThisEvent);
			} else {
				blinkCount = 0;
			}
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
	PostReceiverService(ThisEvent);
}

void ce(Level_t Level) {
	if (Level == LOW) {
		LATBbits.LATB15 = 0;
	} else if (Level == HIGH) {
		LATBbits.LATB15 = 1;
	}
}
