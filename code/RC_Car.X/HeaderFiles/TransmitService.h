#ifndef TransmitService_H
#define TransmitService_H

#include "nRF24L01.h"

// delays
#define DELAY_BTW_BYTES	0
#define CS_LOW_DELAY	15
#define CS_HIGH_DELAY	13

typedef union {
	struct {
		uint8_t TX_FULL		: 1;
		uint8_t RX_P_NO		: 3;
		uint8_t MAX_RT		: 1;
		uint8_t TX_DS		: 1;
		uint8_t RX_DR		: 1;
	};
	struct {
		uint8_t w			: 8;
	};
} SPISTATUSbits_t;

typedef enum {
	LOW = 0,
	HIGH
} Level_t;

typedef enum {
	ES_INIT = 0,
	ES_STATUS_FLAGS,
	ES_HANDLE_PAYLOAD,
	ES_CONTROL_UPDATE
} ES_EventType_t;

typedef struct ES_Event {
  ES_EventType_t EventType;
  uint16_t EventParam;
} ES_Event_t;

// Public Function Prototypes
Radio_t InitTransmitService();
ES_Event_t RunTransmitService(ES_Event_t ThisEvent);

// misc functions
void PackagePayload(uint8_t motorSpeed, uint8_t servoPos);
void ce(Level_t Level);
bool radioIsTransmitter(void);
void TransmitPayload(void);

#endif /* ServTemplate_H */
