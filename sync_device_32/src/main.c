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
	
	start_ote_timer();

	uint32_t i = 0;
	while (1)
	{
		Pulse* p_pulse = &pulse_table[i];
		if (p_pulse->pending && tc_read_cv(OTE_TC, OTE_TC_CH) > p_pulse->timestamp)
		{
			ioport_set_pin_level(p_pulse->pin, 1 - p_pulse->polarity);
			p_pulse->pending = false;
		}
		i++;
		i = (i >= 10) ? 0 : i;
	}
}

