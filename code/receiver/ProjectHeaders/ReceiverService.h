#ifndef ReceiverService_H
#define ReceiverService_H

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
bool InitReceiverService(uint8_t Priority);
bool PostReceiverService(ES_Event_t ThisEvent);
ES_Event_t RunReceiverService(ES_Event_t ThisEvent);

// misc functions
void PackagePayload(uint8_t motorSpeed, uint8_t servoPos);
void ce(Level_t Level);
void TransmitPayload(void);

#endif /* ServTemplate_H */
