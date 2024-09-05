/**
 * \file
 *
 * \brief Synchronization device for laser shutters and cameras
 *
 */

#include <asf.h>


int main(void)
{
	// Initialize the board and the IOPORT system
	sysclk_init();   // Initialize the system clock
	board_init();    // Initialize the board (configures default pins)
	ioport_init();   // Initialize the IOPORT system

	// Configure the pin as an output (Arduino Due uses the PIO ports directly)
	ioport_set_pin_dir(PIO_PA24_IDX, IOPORT_DIR_OUTPUT);  // Set pin PA7 (Arduino pin 11) as output

	while (1)
	{
		// Set the pin high
		ioport_set_pin_level(PIO_PA24_IDX, true);
		delay_ms(1000);  // Wait for 1 second
		
		// Set the pin low
		ioport_set_pin_level(PIO_PA24_IDX, false);
		delay_ms(1000);  // Wait for 1 second
	}
}
