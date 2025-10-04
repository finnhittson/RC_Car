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
The receiver schematic is shown below. For the receiver, the PIC16 communicates with the nRF24L01 radio module through a SPI bus for receiving commands that are then decoded into a PWM signals for motor and servo control. The L9110 h-bridge controls the motor and the LM2937 linear voltage regulator creates the 3.3V supply for the PIC16, nRF24L01 radio module, lights, and servo power. The lights part of the schematic act as the headlights of the car.

bill of materisl
