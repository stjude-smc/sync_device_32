/**
 * @file sys_globals.h
 * @author Roman Kiselev (roman.kiselev@stjude.org)
 * @brief Global system definitions: pinouts and settings.
 */
#pragma once
#include <ioport.h>

#define VERSION "0.6.0\n"


/************************************************************************/
/*                    PINOUT AND WIRING DEFINITIONS                     */
/************************************************************************/
// Camera trigger
#define CAMERA_PIN	"D13"

// Fluidics trigger
#define FLUIDIC_PIN "D2"

// Laser shutters (see pio_sam3x8e.h for pin names) - these have to be on the same port!
#define CY2_PIN		"A0"
#define CY3_PIN		"A1"
#define CY5_PIN		"A2"
#define CY7_PIN		"A3"

#define SHUTTERS_MASK (ioport_pin_to_mask(CY2_PIN) | \
                       ioport_pin_to_mask(CY3_PIN) | \
					   ioport_pin_to_mask(CY5_PIN) | \
					   ioport_pin_to_mask(CY7_PIN))
#define SHUTTERS_PORT ioport_pin_to_port_id(CY2_PIN)


/************************************************************************/
/*                    UART AND DMA CONFIGURATION                        */
/************************************************************************/
#define UART_BUFFER_SIZE 256   // Size of DMA-controlled UART buffers
#define UART_BAUDRATE 115200   // bits per second
#define UART_TIMEOUT  25       // ms
// UART uses timer 8 (module TC2 channel 2)
#define UART_TC              TC2
#define UART_TC_CH           2
#define ID_UART_TC           ID_TC8
#define UART_TIMEOUT_Handler TC8_Handler
#define UART_TC_IRQn	     TC8_IRQn


/************************************************************************/
/*    SYSTEM TIMER CONFIGURATION AND TIME CONVERSION FUNCTIONS          */
/************************************************************************/

// 16bit timer prescaler configuration. See SAM3X TC_CMR register, datasheet p883
#define PRESC32

#ifdef PRESC2    // 1ct = 24ns, overflow after 102s  (1min 42s)
#define TC_CMR_TCCLKS_TIMER_CLOCK TC_CMR_TCCLKS_TIMER_CLOCK1 
#define TC_CONVERSION_MULTIPLIER 420000
#endif

#ifdef PRESC8    // 1ct = 95ns, overflow after 409s  (6min 49s)
#define TC_CMR_TCCLKS_TIMER_CLOCK TC_CMR_TCCLKS_TIMER_CLOCK2
#define TC_CONVERSION_MULTIPLIER 105000
#endif

#ifdef PRESC32   // 1ct = 381ns, overflow after 1636s (27min 16s)
#define TC_CMR_TCCLKS_TIMER_CLOCK TC_CMR_TCCLKS_TIMER_CLOCK3
#define TC_CONVERSION_MULTIPLIER 26520
#endif

#ifdef PRESC128  // 1ct = 1524ns, overflow after 6544s (1h 49min)
#define TC_CMR_TCCLKS_TIMER_CLOCK TC_CMR_TCCLKS_TIMER_CLOCK4
#define TC_CONVERSION_MULTIPLIER 65625
#endif

// microseconds to counts
static inline uint32_t us2cts(uint32_t us) {
	// Avoid overflow by using a larger intermediate type
	uint64_t temp = (uint64_t) us * TC_CONVERSION_MULTIPLIER;
	return (uint32_t)(temp / 10000);
}

// counts to microseconds
static inline uint32_t cts2us(uint32_t cts) {
	uint64_t temp = (uint64_t) cts * 10000;
	return (uint32_t)(temp / TC_CONVERSION_MULTIPLIER);
}


// Main timer/counter for events
#define SYS_TC			TC0
#define SYS_TC_CH		0
#define ID_SYS_TC		ID_TC0
#define SYS_TC_Handler  TC0_Handler
#define SYS_TC_IRQn		TC0_IRQn

// Maximum allowed number of events in the event table
#define MAX_N_EVENTS	1024