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
	

	uint8_t byte;
	while (1)
	{
		poll_UART();
/*		if (sd_rx_byte(&byte) == OK)
		{
			ioport_set_pin_level(CY5_PIN, 0);
			sd_send_chr(byte + 1);
		} else {
			ioport_set_pin_level(CY5_PIN, 1);
		}*/
		// poll_UART();
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



