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
#include "sd_events.h"
#include "sd_triggers.h"


// ENTRY POINT
int main(void)
{
	// Initialize the board
	sysclk_init();   // Initialize the system clock
	board_init();    // Initialize the board (configures default pins)

    sd_init_IO();
    sd_init_UART();
	
    // Notify the host that we are ready
	sd_tx("Sync device is ready. Firmware version: ");
    sd_tx(VERSION);
	
	activate_TC1();

	while (1)
	{
		;
	}
}

