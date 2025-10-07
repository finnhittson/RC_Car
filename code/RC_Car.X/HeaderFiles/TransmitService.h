#ifndef TransmitService_H
#define TransmitService_H

#include "nRF24L01.h"

// delays
#define DELAY_BTW_BYTES	0
#define CS_LOW_DELAY	15
#define CS_HIGH_DELAY	13

typedef enum {
    COLLECT_ADC_DATA,
    TRANSMIT_ADC_DATA,
    CLEAR_INTERRUPT,
    READ_RX,
    UPDATE_CONTROLS,
    DONE,
} State_t;

void packagePayload(uint8_t radioID, uint8_t *bytes);
void transmitPayload(uint8_t statusBits);
void setupSPI(Radio_t radioType);
void configureRX(uint8_t *address);
void configureTX(uint8_t *address);
bool controlsChanged(uint8_t *bytes);

#endif /* ServTemplate_H */
