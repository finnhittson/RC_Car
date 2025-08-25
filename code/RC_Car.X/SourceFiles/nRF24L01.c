#include "../HeaderFiles/nRF24L01.h"
#include <stdbool.h>

bool StartRadio(uint8_t channel, uint8_t payloadSize, uint8_t *result) {
	bool ReturnVal = false;
	uint8_t databytes[2];

	// setup retries
	SetupRetries(1500, 15, result);

	// setup RF
	RFSetup(RF_DR_1Mbps, RF_PWR_18dBm, result);

	// activate features
	FeatureTest(result);

	// setup dynamic payload length
	databytes[0] = W_REGISTER | DYNPD;
    databytes[1] = 0x00;
    SendSPI(databytes, result, 2);

	// setup auto acknowledgment for all datapipes
    databytes[0] = W_REGISTER | EN_AA;
	databytes[1] = 0x3F;
    SendSPI(databytes, result, 2);

	// enable RX address for datapipe 0
    databytes[0] = W_REGISTER | EN_RXADDR;
	databytes[1] = 0x03;
    SendSPI(databytes, result, 2);

	// set number of bytes in each RX payload to be 6 bytes
	SetupPayloadSize(payloadSize, result);

	// setup address width
    databytes[0] = W_REGISTER | SETUP_AW;
	databytes[1] = 0x03;
    SendSPI(databytes, result, 2);

	// set channel
    databytes[0] = W_REGISTER | RF_CH;
	databytes[1] = channel & 0x7F; // only use first 7 bits
    SendSPI(databytes, result, 2);

	// clear status interrupts
    databytes[0] = W_REGISTER | SPI_STATUS;
	databytes[1] = 0x70;
    SendSPI(databytes, result, 2);

	// flush TX and RX radio FIFO's
	FlushRX();
	FlushTX();

	// enable cyclic redundancy check with 2 bytes and power up
	ChangeRadioMode(PowerDown, 1, result);

	// read CONFIG register to ensure proper setting
	ReadRegister(CONFIG, result);
    if (result[1] & 0x0c) {
        ReturnVal = true;
    }

	return ReturnVal;
}

void ce(Level_t Level) {
	if (Level == LOW) {
		LATAbits.LATA1 = 0;
	} else if (Level == HIGH) {
		LATAbits.LATA1 = 1;
	}
}

void FlushRX(void) {
	uint8_t bytes[] = {FLUSH_RX};
	uint8_t result[1];
	SendSPI(bytes, result, 1);
}

void FlushTX(void) {
	uint8_t bytes[] = {FLUSH_TX};
	uint8_t result[1];
	SendSPI(bytes, result, 1);
}

void RFSetup(RF_DR_t datarate, RF_PWR_t power, uint8_t *result) {
	ReadRegister(RF_SETUP, result);
	RF_SETUPbits_t Setup;
	Setup.w = result[1];
	Setup.RF_PWR = power;
	Setup.RF_DR_LOW = (datarate & 0x02) >> 1;
	Setup.RF_DR_HIGH = datarate & 0x01;
	uint8_t databytes[2];
    databytes[0] = W_REGISTER | RF_SETUP;
    databytes[1] = Setup.w;
    SendSPI(databytes, result, 2);
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
			ce(HIGH);
			break;
		}

	case TX:
		{
			CONFIGbits.PWR_UP = 1;
			CONFIGbits.PRIM_RX = 0;
			ce(HIGH);
			break;
		}

	case Standby2:
		{
			// TX FIFO needs to be empty to enter Standby2 mode
			// otherwise will go into TX mode
			CONFIGbits.PWR_UP = 1;			
			CONFIGbits.PRIM_RX = 0;
			ce(HIGH);
			break;
		}

	case Standby1:
		{
			CONFIGbits.PWR_UP = 1;			
			CONFIGbits.PRIM_RX = 0;
			ce(LOW);
			break;
		}

	case PowerDown:
		{
			CONFIGbits.PWR_UP = 0;			
			CONFIGbits.PRIM_RX = 0;
			ce(LOW);
			break;
		}
	}
	uint8_t databytes[2];
    databytes[0] = W_REGISTER | CONFIG;
    databytes[1] = CONFIGbits.w;
    SendSPI(databytes, result, 2);
	delay(POWER_UP_DELAY);
}

void SetupPayloadSize(uint8_t size, uint8_t *result) {
	uint8_t databytes[2];
    databytes[0] = W_REGISTER | RX_PW_P0;
    databytes[1] = size;
    SendSPI(databytes, result, 2);
    
    databytes[0] = W_REGISTER | RX_PW_P1;
    SendSPI(databytes, result, 2);
    
    databytes[0] = W_REGISTER | RX_PW_P2;
    SendSPI(databytes, result, 2);
    
    databytes[0] = W_REGISTER | RX_PW_P3;
    SendSPI(databytes, result, 2);
    
    databytes[0] = W_REGISTER | RX_PW_P4;
    SendSPI(databytes, result, 2);
    
    databytes[0] = W_REGISTER | RX_PW_P5;
    SendSPI(databytes, result, 2);
}

void FeatureTest(uint8_t *result) {
	uint8_t FeaturesBefore[2];
	ReadRegister(FEATURE, FeaturesBefore);

	// see datasheet for nRF24L01 for this mystery command
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
        uint8_t databytes[2] = {ACTIVATE, 0x73};
		if (FeaturesBefore[1] == FeaturesAfter[1]) {
			SendSPI(databytes, result, 2);
		}
        databytes[0] = FEATURE;
		databytes[1] = 0x00;
		SendSPI(databytes, result, 2);
    }
}

void SetupRetries(uint16_t autoReTXDelay, uint8_t autoReTXCount, uint8_t *result) {
	uint16_t ARDbin = autoReTXDelay / 250 - 1;
	if (autoReTXDelay > 4000) {
		ARDbin = 15;
	}
	uint8_t databytes[2];
    databytes[0] = W_REGISTER | SETUP_RETR;
    databytes[1] = (uint8_t)(ARDbin << 4 | autoReTXCount);
    SendSPI(databytes, result, 2);
}

bool ReadRXFIFO(uint8_t *result) {
	bool ReturnVal = false;
	uint8_t bytes[PAYLOAD_SIZE + 1];
	bytes[0] = R_RX_PAYLOAD;
	for (int i = 1; i < PAYLOAD_SIZE + 1; i++) {
		bytes[i] = SPI_NOP;
	}
	SendSPI(bytes, result, PAYLOAD_SIZE + 1);

	// clear interrupts
	uint8_t databytes[2];
    databytes[0] = W_REGISTER | SPI_STATUS;
    databytes[1] = 0x70;
    SendSPI(databytes, result, 2);

	if (result[1] + result[2] + result[3] + result[4] == 0xFF) {
		ReturnVal = true;
	}
	return ReturnVal;
}

void StartListening(uint8_t *address, uint8_t addressWidth, uint8_t *result) {
	// power up radio to RX mode with 2 bytes cyclic redundancy check
	ChangeRadioMode(RX, 1, result);

	// clear interrupts
    uint8_t databytes[6];
    databytes[0] = W_REGISTER | SPI_STATUS;
    databytes[1] = 0x70;
    SendSPI(databytes, result, 2);

	// set address
    databytes[0] = W_REGISTER | RX_ADDR_P0;
    databytes[1] = address[0];
    databytes[2] = address[1];
    databytes[3] = address[2];
    databytes[4] = address[3];
    databytes[5] = address[4];
    SendSPI(databytes, result, 6);
}
