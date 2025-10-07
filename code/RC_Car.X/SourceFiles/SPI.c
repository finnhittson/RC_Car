#include "../HeaderFiles/SPI.h"

// reads a register from the nRF24L01 radio given register address and an array to store in
void ReadRegister(uint8_t reg, uint8_t *result) {
	uint8_t bytes[] = {R_REGISTER | reg, SPI_NOP};
	SendSPI(bytes, result, 2);
}

// sends given bytes through SPI bus
void SendSPI(uint8_t bytes[], uint8_t *result, uint8_t n) {
	// if only sending one bytes then can use simplified version
    if (n == 1) {
        // pull CS line low to start tranmission
		LATAbits.LATA4 = 0;
        // load data into SPI buffer
		SSP1BUF = bytes[0];
        // wait till SPI buffer is empty
		while (!SSP1STATbits.BF);
        // load MISO data into result array
		result[0] = SSP1BUF;
        // pull CS line high to stop SPI transmission
		LATAbits.LATA4 = 1;
	} else {
        // pull CS line low to start transmission
		LATAbits.LATA4 = 0;
        // loop through the bytes to send
		for (uint8_t i = 0; i < n; i++) {
            // check that SPI buffer is empty
            while (SSP1STATbits.BF) {
                SSP1BUF;
            }
            // load byte into SPI buffer
			SSP1BUF = bytes[i];
            // wait till transmission is complete
			while (!SSP1STATbits.BF) {
                // do nothing, no body causes stupid warning to occur
            }
            // check for error
            if (SSP1CON1bits.WCOL) {
                SSP1CON1bits.WCOL = 0;
                result[i] = 0xFF;
            } else {
                // if no error then load MISO into result array
                result[i] = SSP1BUF;
            }
		}
        // pull CS line high to stop SPI transmission
		LATAbits.LATA4 = 1;
	}
	delay(TX_DELAY);
}

// blocking code delay function
// to lazy to use timers and interrupts for proper delays and timing stuffs
void delay (volatile int length) {
	while (length >= 0) {
    	length--;
	}
}

// sends 0xFF to nRF24L01. used to check status bits
void sendNOP(uint8_t *result) {
    uint8_t bytes[1] = {SPI_NOP};
    SendSPI(bytes, result, 1);
}