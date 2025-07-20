#include "SPI.h"

// delays
#define DELAY_TIME		100
#define POWER_UP_DELAY	10000

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

typedef enum {
	RECEIVER = 0,
	TRANSMITTER
} Radio_t;

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
	RX,
	TX,
	Standby2,
	Standby1,
	PowerDown
} Mode;

bool StartRadio(uint8_t channel, uint8_t payloadSize, uint8_t *result);
void FlushTX(void);
void FlushRX(void);
void SetRFChannel(uint8_t channel, uint8_t *result);
void SetAddress(Radio_t radioType, uint8_t *address, uint8_t addressWidth, uint8_t *result);
void RFSetup(RF_DR_t datarate, RF_PWR_t power, uint8_t *result);
void ChangeRadioMode(Mode newMode, uint8_t CRCbytes, uint8_t *result);
void SetupPayloadSize(uint8_t size, uint8_t *result);
void FeatureTest(uint8_t *result);
void SetupRetries(uint16_t autoReTXDelay, uint8_t AutoReTXCount, uint8_t *result);
bool ReadRXFIFO(uint8_t payloadSize, uint8_t *result);
void StartListening(Radio_t radioType, uint8_t *address, uint8_t addressWidth, uint8_t *result);
void StopListening(Radio_t radioType, uint8_t *address, uint8_t addressWidth, uint8_t *result);
