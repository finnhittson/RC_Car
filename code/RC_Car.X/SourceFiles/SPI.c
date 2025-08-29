#include "../HeaderFiles/SPI.h"

void ReadRegister(uint8_t reg, uint8_t *result) {
	uint8_t bytes[] = {R_REGISTER | reg, SPI_NOP};
	SendSPI(bytes, result, 2);
}

void SendSPI(uint8_t bytes[], uint8_t *result, uint8_t n) {
	if (n == 1) {
		LATCbits.LATC3 = 0;
		SSP1BUF = bytes[0];
		while (!SSP1STATbits.BF);
		result[0] = SSP1BUF;
		LATCbits.LATC3 = 1;
	} else {
		LATCbits.LATC3 = 0;
		for (uint8_t i = 0; i < n; i++) {
            while (SSP1STATbits.BF) {
                SSP1BUF;
            }
			SSP1BUF = bytes[i];
			while (!SSP1STATbits.BF) {
                // do nothing, no body causes stupid warning to occur
            }
            
            if (SSP1CON1bits.WCOL) {
                SSP1CON1bits.WCOL = 0;
                result[i] = 0xFF;
            } else {
                result[i] = SSP1BUF;
            }
		}
		LATCbits.LATC3 = 1;
	}
	delay(TX_DELAY);
}

void delay (volatile int length) {
	while (length >= 0) {
    	length--;
	}
}

void sendNOP(uint8_t *result) {
    uint8_t bytes[1] = {SPI_NOP};
    SendSPI(bytes, result, 1);
}