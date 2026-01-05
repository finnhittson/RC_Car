#include "nRF24L01.h"

bool StartRadio(uint8_t channel, uint8_t payloadSize, uint8_t *result) {
	bool ReturnVal = false;
	uint8_t databytes[1];

	// setup retries
	SetupRetries(1500, 15, result);

	// setup RF
	RFSetup(RF_DR_250Kbps, RF_PWR_0dBm, result);

	// activate features
	FeatureTest(result);

	// setup dynamic payload length
	databytes[0] = 0x00;
	WriteRegister(DYNPD, databytes, result, 1);

	// setup auto acknowledgment for all datapipes
	databytes[0] = 0x3F;
	WriteRegister(EN_AA, databytes, result, 1);

	// enable RX address for datapipe 0
	databytes[0] = 0x03;
	WriteRegister(EN_RXADDR, databytes, result, 1);

	// set number of bytes in each RX payload to be 6 bytes
	SetupPayloadSize(payloadSize, result);

	// setup address width
	databytes[0] = 0x03;
	WriteRegister(SETUP_AW, databytes, result, 1);

	// set channel
	databytes[0] = (channel & 0x7F);
	WriteRegister(RF_CH, databytes, result, 1);

	// clear status interrupts
	databytes[0] = 0x70;
	WriteRegister(STATUS, databytes, result, 1);

	// flush TX and RX radio FIFO's
	FlushRX();
	FlushTX();

	// enable cyclic redundancy check with 2 bytes and powerup
	ChangeRadioMode(PowerDown, 1, result);

	// read CONFIG register to ensure propper setting
	ReadRegister(CONFIG, result);
	CONFIGbits_t CONFIGReg;
	CONFIGReg.w = result[1];

	if (CONFIGReg.EN_CRC && CONFIGReg.CRCO) { // && CONFIGReg.PWR_UP
		ReturnVal = true;
	}

	return ReturnVal;
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

void RFSetup(RF_DR_t datarate, RF_PWR_t power, uint8_t *result) {
	ReadRegister(RF_SETUP, result);
	RF_SETUPbits_t Setup;
	Setup.w = result[1];
	Setup.RF_PWR = power;
	Setup.RF_DR_LOW = (datarate & 0x02) >> 1;
	Setup.RF_DR_HIGH = datarate & 0x01;
	uint8_t databytes[] = {Setup.w};
	WriteRegister(RF_SETUP, databytes, result, 1);
}

void ChangeRadioMode(Mode newMode, uint8_t CRCbytes, uint8_t *result) {
	CONFIGbits_t CONFIGbits = {0};
	CONFIGbits.EN_CRC = 1;
	CONFIGbits.CRCO = CRCbytes;
	CONFIGbits.MASK_RX_DR = 0;
	CONFIGbits.MASK_TX_DS = 0;
	CONFIGbits.MASK_MAX_RT = 0;
	switch (newMode) {
	case RX:
		{
			CONFIGbits.PWR_UP = 1;			
			CONFIGbits.PRIM_RX = 1;
			LATBbits.LATB15 = 1;
			break;
		}

	case TX:
		{
			CONFIGbits.PWR_UP = 1;
			CONFIGbits.PRIM_RX = 0;
			LATBbits.LATB15 = 1;
			break;
		}

	case Standby2:
		{
			// TX FIFO needs to be empty to enter Standby2 mode
			// otherwise will go into TX mode
			CONFIGbits.PWR_UP = 1;			
			CONFIGbits.PRIM_RX = 0;
			LATBbits.LATB15 = 1;
			break;
		}

	case Standby1:
		{
			CONFIGbits.PWR_UP = 1;			
			CONFIGbits.PRIM_RX = 0;
			LATBbits.LATB15 = 0;
			break;
		}

	case PowerDown:
		{
			CONFIGbits.PWR_UP = 0;			
			CONFIGbits.PRIM_RX = 0;
			LATBbits.LATB15 = 0;
			break;
		}
	}
	uint8_t databytes[] = { CONFIGbits.w };
	WriteRegister(CONFIG, databytes, result, 1);
	delay(POWER_UP_DELAY);
}

void SetupPayloadSize(uint8_t size, uint8_t *result) {
	uint8_t databytes[] = {size};
	WriteRegister(RX_PW_P0, databytes, result, 1);
	WriteRegister(RX_PW_P1, databytes, result, 1);
	WriteRegister(RX_PW_P2, databytes, result, 1);
	WriteRegister(RX_PW_P3, databytes, result, 1);
	WriteRegister(RX_PW_P4, databytes, result, 1);
	// datapipe 5 is funky, not always written to correctly
	WriteRegister(RX_PW_P5, databytes, result, 1);
}

void FeatureTest(uint8_t *result) {
	uint8_t FeaturesBefore[2];
	ReadRegister(FEATURE, FeaturesBefore);

	uint8_t bytes[] = {ACTIVATE, 0x73};
	SendSPI(bytes, result, 2);

	uint8_t FeaturesAfter[2];
	ReadRegister(FEATURE, FeaturesAfter);

	if (false) {
		// DB_printf("FeaturesBefore = 0x%x\n", FeaturesBefore[1]);
		// DB_printf("FeaturesAfter = 0x%x\n", FeaturesAfter[1]);
		if (FeaturesBefore[1] == FeaturesAfter[1]) {
			// DB_printf("Radio uses nRF24L01+.\n");
		} else {
			// DB_printf("Radio uses nRF24L01.\n");
		}
	}

	if (FeaturesAfter[1]) {
		uint8_t databytes[1] = {0x73};
		if (FeaturesBefore[1] == FeaturesAfter[1]) {
			WriteRegister(ACTIVATE, databytes, result, 1);
		}
		databytes[0] = 0x00;
		WriteRegister(FEATURE, databytes, result, 1);
	}
}

void SetupRetries(uint16_t autoReTXDelay, uint8_t autoReTXCount, uint8_t *result) {
	uint8_t ARDbin = autoReTXDelay / 250 - 1;
	if (autoReTXDelay > 4000) {
		ARDbin = 15;
	}
	uint8_t databytes[] = {ARDbin << 4 | autoReTXCount};
	WriteRegister(SETUP_RETR, databytes, result, 1);
}

bool ReadRXFIFO(uint8_t payloadSize, uint8_t *result) {
	bool ReturnVal = false;
	uint8_t bytes[payloadSize + 1];
	bytes[0] = R_RX_PAYLOAD;
	for (int i = 1; i < payloadSize + 1; i++) {
		bytes[i] = NOP;
	}
	SendSPI(bytes, result, payloadSize + 1);

	// clear interrupts
	uint8_t databytes[] = {0x70};
	WriteRegister(STATUS, databytes, result, 1);

	if (result[1] + result[2] + result[3] + result[4] == 0xFF) {
		ReturnVal = true;
	}
	return ReturnVal;
}

void StartListening(uint8_t *address, uint8_t addressWidth, uint8_t *result) {
	// power up radio to RX mode with 2 bytes cyclic redundancy check
	ChangeRadioMode(RX, 1, result);

	// clear interrupts
	uint8_t databytes[] = {0x70};
	WriteRegister(STATUS, databytes, result, 1);

	// set address
	WriteRegister(RX_ADDR_P0, address, result, addressWidth);
}
