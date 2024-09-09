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


	uint8_t uart_data;
	while (1)
	{
		if (uart_is_rx_ready(UART))
		{
			uart_read(UART, &uart_data);
			sd_send_chr(uart_data + 1);
			ioport_toggle_pin_level(CY5_PIN);
		}
		
/*		// Set the pin high
		ioport_set_pin_level(CY2_PIN, true);
		ioport_set_pin_level(CY3_PIN, false);
		delay_ms(1000);  // Wait for 1 second
		
		// Set the pin low
		ioport_set_pin_level(CY2_PIN, false);
		ioport_set_pin_level(CY3_PIN, true);
		ioport_toggle_pin_level(CY5_PIN);
		delay_ms(1000);  // Wait for 1 second*/
		
		;
		//poll_UART();
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
