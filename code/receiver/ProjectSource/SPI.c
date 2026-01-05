#include "SPI.h"
#include "dbprintf.h"

void ReadRegister(uint8_t reg, uint8_t *result) {
	uint8_t bytes[] = {R_REGISTER | reg, NOP};
	SendSPI(bytes, result, 2);
}

void WriteRegister(uint8_t reg, uint8_t databytes[], uint8_t *result, uint8_t n) {
	uint8_t bytes[n + 1];
	bytes[0] = W_REGISTER | reg;
	for (int i = 0; i < n; i++) {
		bytes[i+1] = databytes[i];
	}
	SendSPI(bytes, result, n+1);
}

void SendSPI(uint8_t bytes[], uint8_t *result, uint8_t n) {
	LATBbits.LATB12 = 0;
	for (uint8_t i = 0; i < n; i++) {
		SPI1BUF = bytes[i];
		while (!SPI1STATbits.SPIRBF);
		result[i] = SPI1BUF;
	}
	LATBbits.LATB12 = 1;
	delay(TX_DELAY);
}

void delay (volatile int length) {
	while (length >= 0) {
    	length--;
	}
}