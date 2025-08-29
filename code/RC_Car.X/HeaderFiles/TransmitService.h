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
	ES_INIT = 0,
	ES_STATUS_FLAGS,
	ES_HANDLE_PAYLOAD,
	ES_CONTROL_UPDATE
} ES_EventType_t;

typedef struct ES_Event {
  ES_EventType_t EventType;
  uint16_t EventParam;
} ES_Event_t;

typedef enum {
    COLLECT_ADC_DATA,
    TRANSMIT_ADC_DATA,
            CLEAR_INTERRUPT,
            READ_RX,
            UPDATE_CONTROLS,
            DONE,
} State_t;

// Public Function Prototypes
Radio_t InitTransmitService();
ES_Event_t RunTransmitService(ES_Event_t ThisEvent);

// misc functions
void packagePayload(uint8_t radioID, uint8_t bytes1, uint8_t bytes2, uint8_t bytes3, uint8_t bytes4);
void transmitPayload(uint8_t statusBits);
void setupSPI(void);
void configureRX(uint8_t *address);
void configureTX(uint8_t *address);

#endif /* ServTemplate_H */
