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



void out_of_memory_handler() {
	// Handle out-of-memory error here
	printf("ERR: out of memory!\n");
	// You can abort or take other actions
	std::abort();
}


extern "C" {
	// Implementation of _write function for printf
	// NOTE: printf is buffered and sends data out after \n symbol
	int _write(int file, char *ptr, int len) {
		sd_tx(ptr, len);
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
}


/************************************************************************/
/*                       ENTRY POINT                                    */
/************************************************************************/
int main() {
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
	
	printf("SYNC DEVICE READY\n");

	while (1) {
		if (!event_queue.empty() && is_sys_timer_running() && current_time_cts() > tc_read_ra(SYS_TC, SYS_TC_CH))
		{
			// we must have missed an event
			process_events();  // <- internally sets RA to timestamp of the next event
		}
	}
}

