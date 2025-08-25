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

#define RADIO_ID			0x01
#define MOTOR_PERIOD		50000
#define AD_CHANNEL1			(1 << 0)
#define AD_CHANNEL2			(1 << 1)

/*---------------------------- Module Functions ---------------------------*/

/*---------------------------- Module Variables ---------------------------*/
uint8_t address[] = {0x30, 0x30, 0x30, 0x31, 0x31};
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

void transmitPayload(SPISTATUSbits_t SPI_STATUSbits) {
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

void packagePayload(uint8_t bytes1, uint8_t bytes2, uint8_t bytes3, uint8_t bytes4) {
	uint8_t checksum = 0xFF - (RADIO_ID + bytes1 + bytes2 + bytes3 + bytes4);
	payload[0] = W_TX_PAYLOAD;
	payload[1] = RADIO_ID;
	payload[2] = bytes1;
	payload[3] = bytes2;
	payload[4] = bytes3;
    payload[5] = bytes4;
    payload[6] = checksum;
}