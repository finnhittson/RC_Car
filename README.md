# 9V Battery RC Car
This repo documents how to make an RC car and transmitter using 3d printted parts, a PIC16LF18325 microcontroller, nRF24L01 RF modules, a 9V battery to power the car and the transmitter, and other basic hardware components. No glue necessary!

## Design Overview
The three main components in building this car are the software, electrical, and the mechanical aspect of the car. The following sections talks about each of these parts in detail.

### Software
Using the PIC16LF18326 as the microcontroller, the driver code was written exclusively in C. The driver code was programmed onto the PIC16 using the MPLAB SNAP and the MPLABX IDE. The software components are broken into the following three files:
1. `main.c`: runs main finite state machine responsible for executing high level transmitter/receiver instructions for controlling motors and servos for the receiver and reading analog control inputs for the trasmitter.
2. `spi.c`: helper SPI functions for transmitting and receiving messages to and from the nRF24L01 radio module.
3. `nRF24L01.c`: helper functions for configuring the nRF24L01 radio module using the `spi.c` functons.

Because this project is intended to be a workshop only one version of code or `main.c` file is written for simplicity. Therefore, for the `main.c` to differentiate between running transmitter code and receiver, a GPIO pin on the micro is dedicated for this indication. The pin under questions is RA5 (pin 2) from the tables below which descripe the pinout for the PIC16 in both the receiver and transmitter. If RA5 is pulled high, then the `main.c` executes transmitter code only. Similarly if RA5 is pulled low, then the `main.c` only executes receiver code. 

| Transmitter Pin Description |             |                |                 | | | Receiver Pin Description    |             |                |                 |
|-----------------------------|-------------|----------------|-----------------|-|-|-----------------------------|-------------|----------------|-----------------|
| **Pin Number**              | **Port**    | **Net Name**   | **Notes**       | | | **Pin Number**              | **Port**    | **Net Name**   | **Notes**       |
| 1                           | $V_{DD}$    | 3.3V           | power supply    | | | 1                           | $V_{DD}$    | 3.3V           | power supply    |
| 2                           | RA5         | 3.3V           | type select     | | | 2                           | RA5         | GND            | type select     |
| 3                           | RA4         | CSN            | SPI bus         | | | 3                           | RA4         | CSN            | SPI bus         |
| 4                           | RA3         | MCLR           | program/NC      | | | 4                           | RA3         | MCLR           | program/NC      |
| 5                           | RC5         | MOSI           | SPI bus         | | | 5                           | RC5         | CE             | radio enable    |
| 6                           | RC4         | SPEED          | pot input       | | | 6                           | RC4         | PWM5.1         | motor PWM       |
| 7                           | RC3         | STEER          | pot input       | | | 7                           | RC3         | PWM6           | servo PWM       |
|                             |             |                |                 | | |                             |             |                |                 |
| 8                           | RC2         | IRQ            | radio interrupt | | | 8                           | RC2         | PWM5           | motor PWM       |
| 9                           | RC1         | MISO           | SPI bus         | | | 9                           | RC1         | MISO           | SPI bus         |
| 10                          | RC0         | SCK            | SPI bus         | | | 10                          | RC0         | SCK            | SPI bus         |
| 11                          | RA2         | CE             | radio enable    | | | 11                          | RA2         | MOSI           | SPI bus         |
| 12                          | RA1         | STEER_TRIM     | pot input       | | | 12                          | RA1         | IRQ            | radio interrupt |
| 13                          | RA0         | LED            | status          | | | 13                          | RA0         | LIGHTS         | status          |
| 14                          | $V_{SS}$    | GND            | power supply    | | | 14                          | $V_{SS}$    | GND            | power supply    |

If the PIC16 detects its a transmitter then it starts the state machine with the inital state of *COLLECT_ADC_DATA*.

### Electrical
The receiver schematic is shown below. For the receiver, the PIC16LF18325 communicates with the nRF24L01 radio module through a SPI bus for receiving commands that are then decoded into a PWM signals for motor and servo control. The PIC16 also controls the motors and servo respectivly and was chosen for its small size, GPIO pin count, and packaging style. The L9110 h-bridge controls the motor and is a very basic h-bridge for driving small motors which make it perfect for this application. The LM2937 linear voltage regulator creates the 3.3V supply for the PIC16, nRF24L01 radio module, lights, and servo power. The lights part of the schematic act as the headlights of the car and are controlled by the PIC16 toggeling the 2N7000 mosfets. 
![Receiver_schematic](schematics/receiver/schematic.png)

The transmitter schematic is shown below. In this instance the PIC16 commuicates with the nRF24L01 radio module to send commands for the receiver. The PIC16 also controls the status LED which indicates power and pairing status. The two modes of control are the two potentiometers which act as the cars accelerator and steering control. There is also one trim pot for the driver to tune the front steering axel so that the car does not drift left or right when driving straight.

![Transmitter_schematic](schematics/transmitter/schematic.png)
bill of materisl
