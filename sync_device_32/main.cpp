/*
 * GccApplication2.cpp
 *
 * Created: 9/17/2024 3:00:55 PM
 * Author : rkiselev
 */ 

extern "C" {
	#include "asf.h"
}

#include <new>  // to set custom out-of-memory handler

#include "globals.h"
#include "uart_comm.h"
#include "events.h"



void activate_watchdog(void) {
	// Calculate the WDT counter value for 1 second timeout
	uint32_t timeout_value = wdt_get_timeout_value(WATCHDOG_TIMEOUT * 1000, BOARD_FREQ_SLCK_XTAL);

	uint32_t wdt_mode = WDT_MR_WDFIEN;
	// Initialize WDT with the calculated timeout value
	wdt_init(WDT, wdt_mode, timeout_value, timeout_value);
	
	NVIC_EnableIRQ(WDT_IRQn);
	NVIC_SetPriority(WDT_IRQn, 0);
}




void out_of_memory_handler() {
	err_led_on();
	// Handle out-of-memory error here
	printf("ERR: out of memory!\n");
	// You can abort or take other actions
	std::abort();
}


extern "C" {
	// Implementation of _write function for printf
	// NOTE: printf is buffered and sends data out after \n symbol
	int _write(int file, char *ptr, int len) {
		uart_tx(ptr, len);
		return len;
	}

	int _read(int file, char *ptr, int len) {
		// This is a minimal implementation; it doesn't actually read any data.
		// It needs to return the number of bytes read (0 in this case).
		return 0;
	}
}


// Initialize predefined pins for camera and laser shutters
void init_pins()
{
	// Initialize all gpio controllers
	sysclk_enable_peripheral_clock(ID_PIOA);
	sysclk_enable_peripheral_clock(ID_PIOB);
	sysclk_enable_peripheral_clock(ID_PIOC);
	sysclk_enable_peripheral_clock(ID_PIOD);
	
	ioport_set_port_dir(SHUTTERS_PORT, SHUTTERS_MASK, IOPORT_DIR_OUTPUT);
	ioport_set_port_level(SHUTTERS_PORT, SHUTTERS_MASK, IOPORT_PIN_LEVEL_LOW);

	ioport_set_pin_dir(pin_name_to_ioport_id(CAMERA_PIN), IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(pin_name_to_ioport_id(CAMERA_PIN), IOPORT_PIN_LEVEL_LOW);

	ioport_set_pin_dir(pin_name_to_ioport_id(FLUIDIC_PIN), IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(pin_name_to_ioport_id(FLUIDIC_PIN), IOPORT_PIN_LEVEL_LOW);

	ioport_set_pin_dir(pin_name_to_ioport_id(ERR_PIN), IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(pin_name_to_ioport_id(ERR_PIN), IOPORT_PIN_LEVEL_LOW);


	ioport_set_pin_dir(DBG_PIN_IDX, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(DBG_PIN_IDX, IOPORT_PIN_LEVEL_LOW);
}

void init_burst_timer()
{
	sysclk_enable_peripheral_clock(ID_TC6); // TC2, channel 0
	tc_init(TC2, 0,
	TC_CMR_TCCLKS_TIMER_CLOCK1 |  // same time unit as the main clock
	TC_CMR_WAVE |          // waveform mode
	TC_CMR_ASWTRG_SET |  // set on timer start
	TC_CMR_ACPA_CLEAR |      // clear on compare event A
	TC_CMR_ACPC_SET |    // set on compare event C
	TC_CMR_WAVSEL_UP_RC    // reset timer on event C
	);
}


/************************************************************************/
/*                       ENTRY POINT                                    */
/************************************************************************/
int main() {
	activate_watchdog();
	
	// Set out of memory handler for `new` operator. This will send error
	// message over UART
	std::set_new_handler(out_of_memory_handler);
	
	// Set buffer size for printf
	setvbuf(stdout, NULL, _IOLBF, UART_BUFFER_SIZE);

	// Initialize all peripheral systems
	sysclk_init();
	board_init();
	init_uart_comm();
	init_pins();	
	init_sys_timer();
	init_burst_timer();
	
	printf("SYNC DEVICE READY\n");

	while (1) {
		if (is_event_missed())
		{
			err_led_on();
			process_events();  // <- internally sets RA to timestamp of the next event
			err_led_off();
		}

		poll_uart();

		wdt_restart(WDT); // Kick the watchdog
	}
}



void WDT_Handler(void)
{
	err_led_on();
	
	printf("ERR - watchdog timeout, restarting system!\n");
	
	delay_ms(50);

	RSTC->RSTC_CR = 0xA5000000 | RSTC_CR_PROCRST;  // processor reset
}