# 9V Battery RC Car
This repo documents how to make an RC car and transmitter using 3d printted parts, a PIC16LF18325 microcontroller, nRF24L01 RF modules, a 9V battery to power the car and the transmitter, and other basic hardware components. No glue necessary!

## Design Overview
The three main components in building this car are the software, electrical, and the mechanical aspect of the car. The following sections talks about each of these parts in detail.

### Software
Using the PIC16LF18326 as the microcontroller, the driver code was written exclusively in C. The driver code was programmed onto the PIC16 using the MPLAB SNAP and the MPLABX IDE. The software components are broken into the following three files:
1. `main.c`: runs main finite state machine responsible for executing high level transmitter/receiver instructions for controlling motors and servos for the receiver and reading analog control inputs for the trasmitter.
2. `spi.c`: helper SPI functions for transmitting and receiving messages to and from the nRF24L01 radio module.
3. `nRF24L01.c`: helper functions for configuring the nRF24L01 radio module using the `spi.c` functons.

### Electrical
   
