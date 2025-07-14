/****************************************************************************

Header file for template service
based on the Gen 2 Events and Services Framework

****************************************************************************/

#ifndef TransmitService_H
#define TransmitService_H

#include "ES_Types.h"

// delays
#define DELAY_TIME		100
#define POWER_UP_DELAY	10000
#define TX_DELAY		45
#define DELAY_BTW_BYTES	0
#define CS_LOW_DELAY	15
#define CS_HIGH_DELAY	13

// SPI commands
#define R_REGISTER 			0x00
#define W_REGISTER 			0x20
#define REGISTER_MASK		0x1F
#define ACTIVATE			0x50
#define R_RX_PL_WID			0x60
#define R_RX_PAYLOAD		0x61
#define W_TX_PAYLOAD		0xA0
#define W_ACK_PAYLOAD		0xA8
#define W_TX_PAYLOAD_NO_ACK	0xB0
#define FLUSH_TX			0xE1
#define FLUSH_RX			0xE2
#define RESUE_TX_PL			0xE3
#define	NOP					0xFF

// register addresses
#define CONFIG 				0x00
#define EN_AA				0x01
#define SETUP_AW			0x03
#define EN_RXADDR			0x02
#define SETUP_RETR			0x04
#define RF_CH				0x05
#define RF_SETUP			0x06
#define STATUS				0x07
#define OBSERVE_TX			0x08
#define RPD					0x09
#define RX_ADDR_P0			0x0A
#define RX_ADDR_P1			0x0B
#define RX_ADDR_P2			0x0C
#define RX_ADDR_P3			0x0D
#define RX_ADDR_P4			0x0E
#define RX_ADDR_P5			0x0F
#define TX_ADDR				0x10
#define RX_PW_P0			0x11
#define RX_PW_P1			0x12
#define RX_PW_P2			0x13
#define RX_PW_P3			0x14
#define RX_PW_P4			0x15
#define RX_PW_P5			0x16
#define FIFO_STATUS			0x17
#define DYNPD				0x1C
#define FEATURE				0x1D

// misc
#define MAX_RETRIES			5
#define RADIO_ID			0x01

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

typedef enum {
	RECEIVER = 0,
	TRANSMITTER
} Radio_t;

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
void SetupPayloadSize(uint8_t size);
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
