/*----------------------------- Include Files -----------------------------*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "TransmitService.h"
#include "dbprintf.h"

/*----------------------------- Module Defines ----------------------------*/
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
#define MAX_RETRIES		5
/*---------------------------- Module Functions ---------------------------*/

/*---------------------------- Module Variables ---------------------------*/
static uint8_t MyPriority;
const uint8_t AddressWidth = 5;
uint8_t Address[] = {0x01, 0x02, 0x03, 0x04, 0x05};

/*------------------------------ Module Code ------------------------------*/
bool InitTransmitService(uint8_t Priority) {
	clrScrn();
	DB_printf("Init Transmit Service\n");
	ES_Event_t ThisEvent;
	MyPriority = Priority;

	// LED config (pin 14)
	TRISBbits.TRISB5 = 0;
	LATBbits.LATB5 = 1;

	// radio CE config (pin 12)
	TRISAbits.TRISA4 = 0;
	LATAbits.LATA4 = 0;

	// SPI config
	if (true) {
		DB_printf("Configuring SPI bus\n");
		SPI1CONbits.ON = 0;         // disable SPI1
		SPI1CONbits.MCLKSEL = 0;    // Configure the SPI clock to be based on PBCLK
		SPI1CONbits.FRMEN = 0;      // Disable the Framed mode
		SPI1CON2bits.AUDEN = 0;     // Disable the Audio mode

		SPI1CONbits.MSTEN = 1;      // select master mode
		SPI1CONbits.SMP = 0;        // input data sampled at middle of data output time

		// SCLK config (RB14 - pin 25)
		TRISBbits.TRISB14 = 0;      // configure as digital input

		// MISO/SDI config (RB11 - pin 22)
		TRISBbits.TRISB11 = 1;      // set MISO as digital input
		SDI1R = 0b0011;             // map SDI1 to RB11

		// MOSI/SDO config (RB13 - pin 24)
		TRISBbits.TRISB13 = 0;      // set MOSI as digital output
		RPB13R = 0b0011;            // map RB13 to SDO1

		// CS config (RB15 - pin 26)
		TRISBbits.TRISB15 = 0;		// set CS as digital output
		LATBbits.LATB15 = 1;
		// RPB15R = 0b0011;

		SPI1BRG = 85;  				// set baud rate SPI1BRG = 2 = 20MHz / (2 * 8MHz - 1)
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

	delay(POWER_UP_DELAY);
	
	// start radio
	bool RadioStarted = false;
	if (!StartRadio()) {
		DB_printf("Unsuccessfully communicated with radio.\n");
	} else {
		DB_printf("Successfully communicated with radio.\n");
		RadioStarted = true;
	}

	if (RadioStarted) {
		// set radio address
		SetAddress(Address);

		ThisEvent.EventType = ES_INIT;
		if (ES_PostToService(MyPriority, ThisEvent) == true) {
			return true;
		} else {
			return false;
		}
	}
}

bool PostTransmitService(ES_Event_t ThisEvent) {
	return ES_PostToService(MyPriority, ThisEvent);
}

ES_Event_t RunTransmitService(ES_Event_t ThisEvent) {
	ES_Event_t ReturnEvent;
	ReturnEvent.EventType = ES_NO_EVENT;

	return ReturnEvent;
}

/***************************************************************************
private functions
***************************************************************************/

bool StartRadio(void) {
	bool ReturnVal = false;
	uint8_t databytes[1];

	// setup retries
	SetupRetries(1500, 15);

	// setup RF
	RFSetup(RF_DR_1Mbps, RF_PWR_0dBm);

	// activate features
	FeatureTest();

	// setup dynamic payload length
	databytes[0] = 0x00;
	WriteRegister(DYNPD, databytes, 1);

	// setup auto acknowledgment for all datapipes
	databytes[0] = 0x3F;
	WriteRegister(EN_AA, databytes, 1);

	// enable RX address for datapipe 0
	databytes[0] = 0x01;
	WriteRegister(EN_RXADDR, databytes, 1);

	// set number of bytes in each RX payload to be 6 bytes
	SetupPayloadSize(6);

	// setup address width
	databytes[0] = AddressWidth;
	WriteRegister(SETUP_AW, databytes, 1);

	// set channel
	SetRFChannel(42);

	// clear status interrupts
	databytes[0] = 0x70;
	WriteRegister(STATUS, databytes, 1);

	// flush TX and RX radio FIFO's
	FlushRX();
	FlushTX();

	// enable cyclic redundancy check with 2 bytes and powerup
	ChangeRadioMode(Standby1, 1);

	// read CONFIG register to ensure propper setting
	uint8_t result[2];
	ReadRegister(CONFIG, result);
	CONFIGbits_t CONFIGReg;
	CONFIGReg.w = result[1];

	if (CONFIGReg.EN_CRC && CONFIGReg.CRCO && CONFIGReg.PWR_UP) {
		ReturnVal = true;
	}

	return ReturnVal;
}

void SetAddress(uint8_t *Address) {
	WriteRegister(RX_ADDR_P0, Address, AddressWidth);
	WriteRegister(TX_ADDR, Address, AddressWidth);
}

void FlushRX(void) {
	uint8_t bytes[] = {FLUSH_RX};
	uint8_t result[1];
	SendSPI(bytes, result, 1);
	delay(DELAY_TIME);
}

void FlushTX(void) {
	uint8_t bytes[] = {FLUSH_TX};
	uint8_t result[1];
	SendSPI(bytes, result, 1);
	delay(DELAY_TIME);
}

void SetRFChannel(uint8_t Channel) {
	uint8_t databytes[] = {Channel & 0x7F};
	WriteRegister(RF_CH, databytes, 1);
}

void SetupPayloadSize(uint8_t PayloadSize) {
	uint8_t databytes[] = {PayloadSize};
	WriteRegister(RX_PW_P0, databytes, 1);
	WriteRegister(RX_PW_P1, databytes, 1);
	WriteRegister(RX_PW_P2, databytes, 1);
	WriteRegister(RX_PW_P3, databytes, 1);
	WriteRegister(RX_PW_P4, databytes, 1);
	// datapipe 5 is funky, not always written to correctly
	WriteRegister(RX_PW_P5, databytes, 1);
}

void FeatureTest(void) {
	uint8_t FeaturesBefore[2];
	ReadRegister(FEATURE, FeaturesBefore);

	uint8_t databytes[1] = {0x73};
	WriteRegister(ACTIVATE, databytes, 1);

	uint8_t FeaturesAfter[2];
	ReadRegister(FEATURE, FeaturesAfter);

	if (false) {
		DB_printf("FeaturesBefore = 0x%x\n", FeaturesBefore[1]);
		DB_printf("FeaturesAfter = 0x%x\n", FeaturesAfter[1]);
		if (FeaturesBefore[1] == FeaturesAfter[1]) {
			DB_printf("Radio uses nRF24L01+.\n");
		} else {
			DB_printf("Radio uses nRF24L01.\n");
		}
	}

	if (FeaturesAfter[1]) {
		uint8_t databytes[1] = {0x73};
		if (FeaturesBefore[1] == FeaturesAfter[1]) {
			WriteRegister(ACTIVATE, databytes, 1);
		}
		databytes[0] = 0x00;
		WriteRegister(FEATURE, databytes, 1);
	}
}

void SetupRetries(uint16_t AutoRetransmitDelay, uint8_t AutoRetransmitCount) {
	uint8_t ARDbin = AutoRetransmitDelay / 250 - 1;
	if (AutoRetransmitDelay > 4000) {
		ARDbin = 15;
	}
	uint8_t databytes[] = {ARDbin << 4 | AutoRetransmitCount};
	WriteRegister(SETUP_RETR, databytes, 1);
}

void RFSetup(RF_DR_t Datarate, RF_PWR_t Power) {
	uint8_t result[2];
	ReadRegister(RF_SETUP, result);

	RF_SETUPbits_t Setup;
	Setup.w = result[1];
	Setup.RF_PWR = Power;
	Setup.RF_DR_LOW = (Datarate & 0x02) >> 1;
	Setup.RF_DR_HIGH = Datarate & 0x01;
	uint8_t databytes[] = {Setup.w};
	WriteRegister(RF_SETUP, databytes, 1);
}

void ReadRegister(uint8_t reg, uint8_t *result) {
	uint8_t bytes[] = {R_REGISTER | reg, NOP};
	SendSPI(bytes, result, 2);
	delay(DELAY_TIME);
}

void WriteRegister(uint8_t reg, uint8_t databytes[], uint8_t n) {
	uint8_t bytes[n + 1];
	bytes[0] = W_REGISTER | reg;
	for (int i = 0; i < n; i++) {
		bytes[i+1] = databytes[i];
	}
	uint8_t result[n+1];
	SendSPI(bytes, result, n+1);
	delay(DELAY_TIME);
}

void SendSPI(uint8_t bytes[], uint8_t *result, uint8_t n) {
	if (n == 1) {
		LATBbits.LATB15 = 0;
		// delay(CS_LOW_DELAY);

		SPI1BUF = bytes[0];
		while (!SPI1STATbits.SPIRBF);
		result[0] = SPI1BUF;

		// delay(CS_HIGH_DELAY);
		LATBbits.LATB15 = 1;
	} else {
		LATBbits.LATB15 = 0;
		// delay(CS_LOW_DELAY);

		for (uint8_t i = 0; i < n; i++) {
			// DB_printf("bytes[%d] = 0x0%x!\n", i, bytes[i]);
			SPI1BUF = bytes[i];
			while (!SPI1STATbits.SPIRBF);
			result[i] = SPI1BUF;
			if (i + 1 != n) {
				// delay(DELAY_BTW_BYTES);
			}
		}

		// delay(CS_HIGH_DELAY);
		LATBbits.LATB15 = 1;
	}
}

void ChangeRadioMode(Mode newMode, uint8_t CRCbytes) {
	CONFIGbits_t CONFIGbits = {0};
	CONFIGbits.EN_CRC = 1;
	CONFIGbits.CRCO = CRCbytes;
	// CONFIGbits.MASK_RX_DR = 1;
	// CONFIGbits.MASK_TX_DS = 1;
	// CONFIGbits.MASK_MAX_RT = 1;

	switch (newMode) {
	case RX:
		{
			DB_printf("Configuring radio into RX mode.\n");
			CONFIGbits.PWR_UP = 1;			
			CONFIGbits.PRIM_RX = 1;
			LATAbits.LATA4 = 1;
			break;
		}

	case TX:
		{
			DB_printf("Configuring radio into TX mode.\n");
			CONFIGbits.PWR_UP = 1;
			CONFIGbits.PRIM_RX = 0;
			LATAbits.LATA4 = 1;
			break;
		}

	case Standby2:
		{
			DB_printf("Configuring radio into Standby2 mode.\n");
			// TX FIFO needs to be empty to enter Standby2 mode
			// otherwise will go into TX mode
			CONFIGbits.PWR_UP = 1;			
			CONFIGbits.PRIM_RX = 0;
			LATAbits.LATA4 = 1;
			break;
		}

	case Standby1:
		{
			DB_printf("Configuring radio into Standby1 mode.\n");
			CONFIGbits.PWR_UP = 1;			
			CONFIGbits.PRIM_RX = 0;
			LATAbits.LATA4 = 0;
			break;
		}

	case PowerDown:
		{
			DB_printf("Configuring radio into PowerDown mode.\n");
			CONFIGbits.PWR_UP = 0;			
			CONFIGbits.PRIM_RX = 0;
			LATAbits.LATA4 = 0;
			break;
		}
	}
	// DB_printf("0x%x\n", CONFIGbits.w);
	uint8_t databytes[] = { CONFIGbits.w };
	WriteRegister(CONFIG, databytes, 1);
}

void PrintStatus(STATUSbits_t STATUSbits) {
	DB_printf("\nSTATUS Register\n");
	DB_printf("RX_DR (bit 6)\t\t= %d (data ready FX FIFO interrupt)\n", STATUSbits.RX_DR);
	DB_printf("TX_DS (bit 5)\t\t= %d (data sent TX FIFO interrupt)\n", STATUSbits.TX_DS);
	DB_printf("MAX_RT (bit 4)\t\t= %d (max # TX retransmits interrupt)\n", STATUSbits.MAX_RT);
	DB_printf("RX_P_NO (bit 3:1)\t= %d (0-5: data pipe #, 6: NA, 7: RX FIFO empty)\n", STATUSbits.RX_P_NO);
	DB_printf("TX_FULL (bit 0)\t\t= %d (1: TX FIFO full, 0: TX FIFO not full)\n\n", STATUSbits.TX_FULL);
}

void delay (volatile int length) {
	while (length >= 0) {
    	length--;
	}
}