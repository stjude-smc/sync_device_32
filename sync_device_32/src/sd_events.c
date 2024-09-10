/*
 * sd_events.c
 *
 * Created: 9/9/2024 5:09:35 PM
 *  Author: rkiselev
 */ 

#include <asf.h>
#include "sd_events.h"


void activate_TC1(void)
{
	sysclk_enable_peripheral_clock(ID_TC3);
	
	tc_init(TC1, 0,
			TC_CMR_TCCLKS_TIMER_CLOCK1 |   // Prescaler MCK/2
			TC_CMR_WAVSEL_UP_RC            // Count up
	);
	
	uint32_t rc_value = (sysclk_get_peripheral_hz() / 2 / 1000) * 250;
	tc_write_rc(TC1, 0, rc_value);

	// Enable the interrupt on RC compare
	tc_enable_interrupt(TC1, 0, TC_IER_CPCS);
	
	NVIC_EnableIRQ(TC3_IRQn);
	
	tc_start(TC1, 0);
}

void TC3_Handler(void)
{
	tc_get_status(TC1, 0);
	ioport_toggle_pin_level(CAMERA_PIN);
	tc_start(TC1, 0);
}
