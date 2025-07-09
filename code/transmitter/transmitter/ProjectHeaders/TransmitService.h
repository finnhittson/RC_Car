/****************************************************************************

Header file for template service
based on the Gen 2 Events and Services Framework

****************************************************************************/

#ifndef TransmitService_H
#define TransmitService_H

#include "ES_Types.h"

typedef union {
	struct {
		uint8_t PRIM_RX		: 1;
		uint8_t PWR_UP		: 1;
		uint8_t CRCO		: 1;
		uint8_t EN_CRC		: 1;
		uint8_t MASK_MAX_RT	: 1;
		uint8_t MASK_TX_DS	: 1;
		uint8_t MASK_RX_DR	: 1;
	};
	struct {
		uint8_t w			: 8;
	};
} CONFIGbits_t;

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
} STATUSbits_t;

typedef union {
	struct {
		uint8_t RX_EMPTY	: 1;
		uint8_t RX_FULL		: 1;
		uint8_t				: 2;
		uint8_t TX_EMPTY	: 1;
		uint8_t TX_FULL		: 1;
		uint8_t TX_REUSE	: 1;
	};
	struct {
		uint8_t w			: 8;
	};
} FIFO_STATUSbits_t;

typedef union {
	struct {
		uint8_t 			: 1;
		uint8_t RF_PWR		: 2;
		uint8_t RF_DR_HIGH	: 1;
		uint8_t PLL_HIGH	: 1;
		uint8_t RF_DR_LOW	: 1;
		uint8_t 			: 1;
		uint8_t CONT_WAVE	: 1;
	};
	struct {
		uint8_t w			: 8;
	};
} RF_SETUPbits_t;

typedef enum {
	RX,
	TX,
	Standby2,
	Standby1,
	PowerDown
} Mode;

typedef enum {
	RF_DR_1Mbps = 0,
	RF_DR_2Mbps,
	RF_DR_250Kbps
} RF_DR_t;

typedef enum {
	RF_PWR_18dBm = 0,
	RF_PWR_12dBm,
	RF_PWR_6dBm,
	RF_PWR_0dBm
} RF_PWR_t;

typedef enum {
	Throttle = 0,
	Steering,
	Misc
} Mode_t;

typedef enum {
	LOW = 0,
	HIGH
} Level_t;

// Public Function Prototypes
bool InitTransmitService(uint8_t Priority);
bool PostTransmitService(ES_Event_t ThisEvent);
ES_Event_t RunTransmitService(ES_Event_t ThisEvent);

// spi functions
void SendSPI(uint8_t bytes[], uint8_t *result, uint8_t n);
void ReadRegister(uint8_t reg, uint8_t *result);
void WriteRegister(uint8_t reg, uint8_t databytes[], uint8_t n);

// radio functions
bool StartRadio(void);
void ChangeRadioMode(Mode newMode, uint8_t CRCbytes);
void SetupRetries(uint16_t autoReTXDelay, uint8_t AutoReTXCount);
void RFSetup(RF_DR_t datarate, RF_PWR_t power);
void FeatureTest(void);
void SetupPayloadSize(uint8_t payloadSize);
void SetRFChannel(uint8_t channel);
void FlushTX(void);
void FlushRX(void);
void SetAddress(uint8_t *address);
void StopListening(void);
void TransmitPayload(void);
void StartListening(void);
void ReadRXFIFO(uint8_t *result);

// misc functions
void PrintStatus(STATUSbits_t STATUSbits);
void delay (volatile int length);
void PackagePayload(Mode_t type, uint8_t data1, uint8_t data2);
void InitPayload(void);
void ce(Level_t Level);

#endif /* ServTemplate_H */
