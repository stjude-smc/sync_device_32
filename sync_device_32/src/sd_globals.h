/**
 * @file sys_globals.h
 * @author Roman Kiselev (roman.kiselev@stjude.org)
 * @brief Global system definitions: pin out and settings.
 * @version 0.4
 * @date 2022-01-10
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#include <ioport.h>

#define VERSION "0.6.0\n"


/*********************************
HELPFUL BIT MANIPULATION FUNCTIONS
*********************************/
#define bit(b)                         (1UL << (b))
#define bitRead(register, b)            (((register) >> (b)) & 0x01)
#define bitSet(register, b)             ((register) |= (1UL << (b)))
#define bitClear(register, b)           ((register) &= ~(1UL << (b)))
#define bitToggle(register, b)          ((register) ^= (1UL << (b)))
#define bitWrite(register, b, bitvalue) ((bitvalue) ? bitSet(register, b) : bitClear(register, b))


/****************************
PINOUT AND WIRING DEFINITIONS
****************************/
// Camera trigger
#define CAMERA_PIN	PIO_PB27_IDX // aka Arduino pin 13

// Fluidics trigger
#define FLUIDIC_PIN PIO_PB25_IDX // aka Arduino pin 2

// Laser shutters (see pio_sam3x8e.h for pin names) - these have to be on the same port!
#define CY2_PIN		PIO_PA16_IDX // aka Arduino pin A0
#define CY3_PIN		PIO_PA24_IDX // aka Arduino pin A1
#define CY5_PIN		PIO_PA23_IDX // aka Arduino pin A2
#define CY7_PIN		PIO_PA22_IDX // aka Arduino pin A3

#define SHUTTERS_MASK (ioport_pin_to_mask(CY2_PIN) | ioport_pin_to_mask(CY3_PIN) | ioport_pin_to_mask(CY5_PIN) | ioport_pin_to_mask(CY7_PIN))
#define SHUTTERS_PORT ioport_pin_to_port_id(CY2_PIN)


/*************************
UART AND DMA CONFIGURATION
*************************/

#define UART_BUFFER_SIZE 100   // Size of DMA-controlled UART buffer
#define UART_BAUDRATE 115200