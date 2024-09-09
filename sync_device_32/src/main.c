/**
 * \file
 *
 * \brief Synchronization device for laser shutters and cameras
 *
 */

#include <asf.h>
#include <string.h>
#include "sd_globals.h"
#include "sd_comport.h"

// Declaration of function prototypes
void sd_init_IO(void);

void sd_init_timer0(void);


// ENTRY POINT
int main(void)
{
	// Initialize the board
	sysclk_init();   // Initialize the system clock
	board_init();    // Initialize the board (configures default pins)

    sd_init_IO();
    sd_init_UART();
	
    // Notify the host that we are ready
	sd_send_string("Sync device is ready. Firmware version: ");
    sd_send_string(VERSION);
	
	sd_init_timer0();


	pdc_rx_clear_cnt(uart_get_pdc_base(UART));
	while (1)
	{
		delay_ms(100);
		//ioport_toggle_pin_level(CY5_PIN);
	}
}


void sd_init_IO(void)
{
	// Initialize the IOPORT system
	ioport_init();   // Initialize the IOPORT system

	// Configure the laser shutters as outputs
	ioport_set_port_dir(SHUTTERS_PORT, SHUTTERS_MASK, IOPORT_DIR_OUTPUT);

	// Configure the fluidic trigger as output
	ioport_set_pin_dir(FLUIDIC_PIN, IOPORT_DIR_OUTPUT);

	// Configure the camera trigger as output
	ioport_set_pin_dir(CAMERA_PIN, IOPORT_DIR_OUTPUT);
	
	// Enable built-in LED on PB27 (Arduino pin 13)
}


#define DELAY_MS 250


void sd_init_timer0(void)
{
	sysclk_enable_peripheral_clock(ID_TC0);
	
	tc_init(TC0, 0,
			TC_CMR_TCCLKS_TIMER_CLOCK4 | TC_CMR_WAVSEL_UP_RC);
	
	uint32_t rc_value = (sysclk_get_peripheral_hz() / 128 / 1000) * DELAY_MS;
	tc_write_rc(TC0, 0, rc_value);

    // Enable the interrupt on RC compare
    tc_enable_interrupt(TC0, 0, TC_IER_CPCS);
	
	tc_start(TC0, 0);
	
	NVIC_EnableIRQ(TC0_IRQn);
}

void TC0_Handler(void){
	// Read the status register to clear the interrupt flag
	tc_get_status(TC0, 0);

	// Toggle LED
	ioport_toggle_pin_level(CY5_PIN);


	tc_start(TC0, 0);
}
