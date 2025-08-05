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
#define PAYLOAD_SIZE		4

#define RADIO_ID			0x01
#define MOTOR_PERIOD		50000
#define AD_CHANNEL1			(1 << 0)
#define AD_CHANNEL2			(1 << 1)

/*---------------------------- Module Functions ---------------------------*/

/*---------------------------- Module Variables ---------------------------*/
uint8_t address[] = {0x30, 0x30, 0x30, 0x31, 0x31};
uint8_t payload[PAYLOAD_SIZE + 1];
SPISTATUSbits_t SPI_STATUSbits;
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
	SPI_STATUSbits.w = result[0];
    bool RadioStarted = false;
	// setup radio as receiver
	

	// setup radio as transmitter
	if (RadioStarted && radioType == TRANSMITTER) {
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
		SPI_STATUSbits.w = result[0];

		// power up radio
		ChangeRadioMode(Standby1, 1, result);
		SPI_STATUSbits.w = result[0];
//		DB_printf("Radio configured as a transmitter\n");

		ANSELAbits.ANSA0 = 1;
		TRISAbits.TRISA0 = 1;

		ANSELAbits.ANSA1 = 1;
		TRISAbits.TRISA1 = 1;

//		ADC_ConfigAutoScan(AD_CHANNEL1 | AD_CHANNEL2);
	}

	return radioType;
}

ES_Event_t RunTransmitService(ES_Event_t ThisEvent) {
	ES_Event_t ReturnEvent;
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
			if (SPI_STATUSbits.RX_DR) {
//				DB_printf("Data ready in RX FIFO\n");
				if (!radioIsTransmitter()) {
					ThisEvent.EventType = ES_HANDLE_PAYLOAD;
//					PostTransmitService(ThisEvent);
				}
			} else if (SPI_STATUSbits.TX_DS) {
//				DB_printf("Data successfully sent from TX FIFO\n");
				readyToTransmit = true;
			} else if (SPI_STATUSbits.MAX_RT) {
//				DB_printf("Max number of TX transmits reached\n");
			} else {
//				DB_printf("SHOULD NOT BE HERE!!!\n");
			}
			break;
		}

	case ES_HANDLE_PAYLOAD:
		{
			uint8_t result[PAYLOAD_SIZE + 1];
			bool validData = ReadRXFIFO(result);
//			DB_printf("HERE\n");
			if (result[1] == RADIO_ID) { // && validData
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
			} else if (result[1] != RADIO_ID) {
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
			PackagePayload(motorSpeed, servoPos);
			if (readyToTransmit) {
//				DB_printf("Update motor speed: %d\n", motorSpeed);
//				DB_printf("Update servo pos: %d\n\n", servoPos);
				TransmitPayload();
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
void __interrupt() ISR(void) {
    if (IOCAFbits.IOCAF5) {
        IOCAFbits.IOCAF5 = 0;       // clear flag
        // DB_printf("Interrupt occured\n");
        uint8_t databytes[2];
        uint8_t result[2];
        databytes[0] = W_REGISTER | SPI_STATUS;
        databytes[1] = 0x70;
        SendSPI(databytes, result, 2);
        SPI_STATUSbits.w = result[0];
        ES_Event_t ThisEvent;
        ThisEvent.EventType = ES_STATUS_FLAGS;
        //PostTransmitService(ThisEvent);
    }
}

bool radioIsTransmitter(void) {
	if (radioType == TRANSMITTER) {
		return true;
	}
	return false;
}

void ce(Level_t Level) {
	if (Level == LOW) {
		LATAbits.LATA1 = 0;
	} else if (Level == HIGH) {
		LATAbits.LATA1 = 1;
	}
}

void TransmitPayload(void) {
	readyToTransmit = false;
	// DB_printf("Writing payload to radio\n");
	if (SPI_STATUSbits.w & 0x70) {
		// clear STATUS register to allow for more transmissions
		uint8_t databytes[2];
        uint8_t result[2];
        databytes[0] = W_REGISTER | SPI_STATUS;
        databytes[1] = 0x70;
        SendSPI(databytes, result, 2);
		SPI_STATUSbits.w = result[0];
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
