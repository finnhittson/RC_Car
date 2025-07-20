#ifndef TransmitService_H
#define TransmitService_H

#include "ES_Types.h"
#include "nRF24L01.h"

// delays
#define DELAY_BTW_BYTES	0
#define CS_LOW_DELAY	15
#define CS_HIGH_DELAY	13

typedef enum {
	LOW = 0,
	HIGH
} Level_t;

// Public Function Prototypes
bool InitTransmitService(uint8_t Priority);
bool PostTransmitService(ES_Event_t ThisEvent);
ES_Event_t RunTransmitService(ES_Event_t ThisEvent);

// misc functions
void PackagePayload(uint8_t motorSpeed, uint8_t servoPos);
void ce(Level_t Level);
bool radioIsTransmitter(void);
void TransmitPayload(void);

#endif /* ServTemplate_H */
