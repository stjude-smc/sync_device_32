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
	
	if (timeout_value == WDT_INVALID_ARGUMENT)
	{
		printf("ERR: Can't activate watchdog for timeout of %lu us\n", WATCHDOG_TIMEOUT * 1000);
		return;
	}

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
	
	printf("Sync device is ready. Firmware version: %s\n", VERSION);
	
	start_sys_timer();

	while (1) {
		if (is_event_missed())
		{
			err_led_on();
			process_events();  // <- internally sets RA to timestamp of the next event
			err_led_off();
		}

		poll_uart();

		// Indicates execution of the main loop
		err_led_on();
		err_led_off();

		wdt_restart(WDT); // Kick the watchdog
	}
}



void WDT_Handler(void)
{
	err_led_on();
	const char err_msg[] = "ERR - watchdog timeout, restarting system!\n";
	
	pdc_packet_t uart_tx_packet;
	uart_tx_packet.ul_addr = (uint32_t) err_msg;
	uart_tx_packet.ul_size = sizeof(err_msg);
	pdc_tx_init(uart_get_pdc_base(UART), &uart_tx_packet, nullptr);
	
	delay_ms(50);

	RSTC->RSTC_CR = 0xA5000000 | RSTC_CR_PROCRST;  // processor reset
}