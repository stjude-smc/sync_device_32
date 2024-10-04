/**
 * @file sys_globals.h
 * @author Roman Kiselev (roman.kiselev@stjude.org)
 * @brief Global system definitions: pinouts and settings.
 */
#pragma once

#ifndef UNIT_TEST
#include <ioport.h>
#include "pins.h"
#endif

#define VERSION "0.6.0"


/************************************************************************/
/*                    PINOUT AND WIRING DEFINITIONS                     */
/************************************************************************/
// Camera trigger
#define CAMERA_PIN	"D13" // PIO_PB27_IDX

// Fluidics trigger
#define FLUIDIC_PIN "D2"  // PIO_PB25_IDX

// Laser shutters (see pio_sam3x8e.h for pin names) - these have to be on the same port!
#define CY2_PIN		"A0"  // PIO_PA16_IDX
#define CY3_PIN		"A1"  // PIO_PA24_IDX
#define CY5_PIN		"A2"  // PIO_PA23_IDX
#define CY7_PIN		"A3"  // PIO_PA22_IDX

#define SHUTTERS_MASK (ioport_pin_to_mask(pin_name_to_ioport_id(CY2_PIN)) | \
                       ioport_pin_to_mask(pin_name_to_ioport_id(CY3_PIN)) | \
					   ioport_pin_to_mask(pin_name_to_ioport_id(CY5_PIN)) | \
					   ioport_pin_to_mask(pin_name_to_ioport_id(CY7_PIN)))
#define SHUTTERS_PORT ioport_pin_to_port_id(pin_name_to_ioport_id(CY2_PIN))


/************************************************************************/
/*                    UART AND DMA CONFIGURATION                        */
/************************************************************************/
#define UART_BUFFER_SIZE 512   // Size of DMA-controlled UART buffers
#define UART_BAUDRATE 115200   // bits per second
#define UART_TIMEOUT  25       // ms
// UART uses timer 8 (module TC2 channel 2)
#define UART_TC              TC2
#define UART_TC_CH           2
#define ID_UART_TC           ID_TC8
#define UART_TIMEOUT_Handler TC8_Handler
#define UART_TC_IRQn	     TC8_IRQn


/************************************************************************/
/*                      EVENT HANDLING                                  */
/************************************************************************/
// Maximum allowed number of events in the event table
#define MAX_N_EVENTS	1024

// Uniform time delay added to every single scheduled event, us
// It should be long enough to ensure correct event processing
// under any circumstance
#define UNIFORM_TIME_DELAY 500

// Minimal interval between two subsequent runs of the same events, us
#define MIN_EVENT_INTERVAL 25

// Default pulse duration, us
#define DFL_PULSE_DURATION 100

/************************************************************************/
/*    SYSTEM TIMER CONFIGURATION AND TIME CONVERSION FUNCTIONS          */
/************************************************************************/

// Timer prescaler configuration. See SAM3X TC_CMR register, datasheet p883
// Options are 2, 8, 32, and 128
#define PRESC128

#ifdef PRESC2    // 1ct = 24ns, overflow after 102s  (1min 42s)
#define SYS_TC_PRESCALER 2
#define SYS_TC_CMR_TCCLKS_TIMER_CLOCK TC_CMR_TCCLKS_TIMER_CLOCK1
#endif

#ifdef PRESC8    // 1ct = 95ns, overflow after 409s  (6min 49s)
#define SYS_TC_PRESCALER 8
#define SYS_TC_CMR_TCCLKS_TIMER_CLOCK TC_CMR_TCCLKS_TIMER_CLOCK2
#endif

#ifdef PRESC32   // 1ct = 381ns, overflow after 1636s (27min 16s)
#define SYS_TC_PRESCALER 32
#define SYS_TC_CMR_TCCLKS_TIMER_CLOCK TC_CMR_TCCLKS_TIMER_CLOCK3
#endif

#ifdef PRESC128  // 1ct = 1524ns, overflow after 6544s (1h 49min)
#define SYS_TC_PRESCALER 128
#define SYS_TC_CMR_TCCLKS_TIMER_CLOCK TC_CMR_TCCLKS_TIMER_CLOCK4
#endif

#define SYS_TC_CONVERSION_MULTIPLIER (8400000/SYS_TC_PRESCALER)

// microseconds to counts
static inline uint32_t us2cts(uint32_t us) {
	// Avoid overflow by using a larger intermediate type
	uint64_t temp = (uint64_t) us * SYS_TC_CONVERSION_MULTIPLIER;
	return (uint32_t)(temp / 100000);
}

// counts to microseconds
static inline uint32_t cts2us(uint32_t cts) {
	uint64_t temp = (uint64_t) cts * 100000;
	return (uint32_t)(temp / SYS_TC_CONVERSION_MULTIPLIER);
}


// Main timer/counter for events
#define SYS_TC			TC0
#define SYS_TC_CH		0
#define ID_SYS_TC		ID_TC0
#define SYS_TC_Handler  TC0_Handler
#define SYS_TC_IRQn		TC0_IRQn
