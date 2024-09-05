/**
 * \file
 *
 * \brief Synchronization device for laser shutters and cameras
 *
 */

#include <asf.h>
#include "sys_globals.h"

// Declaration of function prototypes
void outputs_init(void);

// ENTRY POINT
int main(void)
{
	// Initialize the board
	sysclk_init();   // Initialize the system clock
	board_init();    // Initialize the board (configures default pins)

	outputs_init();  // Initialize IO pins

	while (1)
	{
		// Set the pin high
		ioport_set_pin_level(CY2_PIN, true);
		ioport_set_pin_level(CY3_PIN, false);
		delay_ms(1000);  // Wait for 1 second
		
		// Set the pin low
		ioport_set_pin_level(CY2_PIN, false);
		ioport_set_pin_level(CY3_PIN, true);
		ioport_toggle_pin_level(CY5_PIN);
		delay_ms(1000);  // Wait for 1 second
	}
}


void outputs_init(void)
{
	// Initialize the IOPORT system
	ioport_init();   // Initialize the IOPORT system

	// Configure the laser shutters as outputs
	/*
	ioport_set_pin_dir(CY2_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(CY3_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(CY5_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(CY7_PIN, IOPORT_DIR_OUTPUT);
	*/
	ioport_set_port_dir(SHUTTERS_PORT, SHUTTERS_MASK, IOPORT_DIR_OUTPUT);

	// Configure the camera trigger as outputs
	ioport_set_pin_dir(CAMERA_PIN, IOPORT_DIR_OUTPUT);

}
