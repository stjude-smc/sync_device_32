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


void my_pio_handler(uint32_t id, uint32_t mask);
void my_pio_handler2(uint32_t id, uint32_t mask);
void get_pio_and_id(uint32_t pin, Pio** pio, uint32_t* pio_id);
void init_buttons(void);

// Define PC18 as the input pin
#define INPUT_PIN1 PIO_PC12_IDX  // D51
#define INPUT_PIN2 PIO_PC9_IDX   // D41

void f(uint32_t a, uint32_t b);
void f(uint32_t a, uint32_t b){set_lasers(a);}

// Helper function to return the correct PIO bank and ID based on the input pin
void get_pio_and_id(uint32_t pin, Pio** pio, uint32_t* pio_id) {
	if ((pin >= PIO_PA0_IDX) && (pin <= PIO_PA29_IDX)) {
		*pio = PIOA;
		*pio_id = ID_PIOA;
		} else if ((pin >= PIO_PB0_IDX) && (pin <= PIO_PB31_IDX)) {
		*pio = PIOB;
		*pio_id = ID_PIOB;
		} else if ((pin >= PIO_PC0_IDX) && (pin <= PIO_PC30_IDX)) {
		*pio = PIOC;
		*pio_id = ID_PIOC;
		} else if ((pin >= PIO_PD0_IDX) && (pin <= PIO_PD10_IDX)) {
		*pio = PIOD;
		*pio_id = ID_PIOD;
		} else {
			// Handle error or unsupported pin
	}
}

	
// ENTRY POINT
int main(void)
{
	// Initialize the board
	sysclk_init();   // Initialize the system clock
	board_init();    // Initialize the board (configures default pins)

    sd_init_IO();
    sd_init_UART();
	init_sys_timer();
	
		
	// Notify the host that we are ready
	sd_tx("Sync device is ready. Firmware version: ");
	sd_tx(VERSION);
	
	init_buttons();

	Event e = {0};
/*	e.func = f; e.arg1 = 0b0001; e.timestamp=00000; schedule_event(e);
	e.func = f; e.arg1 = 0b0010; e.timestamp=10000; schedule_event(e);
	e.func = f; e.arg1 = 0b0100; e.timestamp=20000; schedule_event(e);
	e.func = f; e.arg1 = 0b1000; e.timestamp=40000; schedule_event(e);
	e.func = f; e.arg1 = 0b0000; e.timestamp=50000; schedule_event(e);	
*/
	start_sys_timer();

	while (1)
	{
		// Housekeeping functions go here
		verify_ra_is_set();
		update_event_table();
	}
}



void my_pio_handler(uint32_t id, uint32_t mask) {
	// Check if the interrupt is for the correct pins
	if (mask == ioport_pin_to_mask(INPUT_PIN1)) {
		//sd_tx("Button 1 event\n");
		
		Event e = {0};
		e.func = f; e.arg1 = 0b0001; e.timestamp=000000; schedule_event(e);
		e.func = f; e.arg1 = 0b0010; e.timestamp=100000; schedule_event(e);
		e.func = f; e.arg1 = 0b0100; e.timestamp=200000; schedule_event(e);
		e.func = f; e.arg1 = 0b1000; e.timestamp=400000; schedule_event(e);
		e.func = f; e.arg1 = 0b0000; e.timestamp=500000; schedule_event(e);
	}
}


void my_pio_handler2(uint32_t id, uint32_t mask) {
	// Check if the interrupt is for the correct pin
	if (mask == ioport_pin_to_mask(INPUT_PIN2)) {
		sd_tx("Button 2 event\n");
	}
}



void init_buttons(void){
	// Variables for the PIO bank and PIO ID
	Pio* pio;
	uint32_t pio_id;
	get_pio_and_id(INPUT_PIN1, &pio, &pio_id);

	// Configure INPUT_PIN as input with pull-up
	ioport_set_pin_dir(INPUT_PIN1, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(INPUT_PIN1, IOPORT_MODE_PULLUP);
	ioport_disable_pin(INPUT_PIN1);  // Release control from IOPORT to PIO

	// Set up the PIO interrupt for the input pin
	pio_handler_set(pio, pio_id, ioport_pin_to_mask(INPUT_PIN1), PIO_IT_EDGE, my_pio_handler);

	// Enable rising and falling edge interrupts
	pio_enable_interrupt(pio, ioport_pin_to_mask(INPUT_PIN1));
	pio_get_interrupt_status(pio);  // Clear pending interrupts
	pio_enable_pin_interrupt(INPUT_PIN1);

	// Enable the PIO interrupt in the NVIC
	NVIC_EnableIRQ((IRQn_Type)pio_id);


	// Variables for the PIO bank and PIO ID
	get_pio_and_id(INPUT_PIN2, &pio, &pio_id);

	// Configure INPUT_PIN as input with pull-up
	ioport_set_pin_dir(INPUT_PIN2, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(INPUT_PIN2, IOPORT_MODE_PULLUP);
	ioport_disable_pin(INPUT_PIN2);  // Release control from IOPORT to PIO

	// Set up the PIO interrupt for the input pin
	pio_handler_set(pio, pio_id, ioport_pin_to_mask(INPUT_PIN2), PIO_IT_EDGE, my_pio_handler2);

	// Enable rising and falling edge interrupts
	pio_enable_interrupt(pio, ioport_pin_to_mask(INPUT_PIN2));
	pio_get_interrupt_status(pio);  // Clear pending interrupts
	pio_enable_pin_interrupt(INPUT_PIN2);

	// Enable the PIO interrupt in the NVIC
	NVIC_EnableIRQ((IRQn_Type)pio_id);
}


/// WHAT'S NEXT?
/*


Implement scheduling for the future.
 Start with scheduling a pin level event
  - define event table
  - activate R compare interrupt
  - add correct table sorting by timestamp and removal of inactive elements
 
 Add pin toggle event
 Then add start/stop clock commands
 
 Re-write pulse scheduling function
  - Remove pulse table from main loop
  - Scheduling a pulse also schedules pin toggle event
 
 Finally, add automatic rescheduling of functions
 based on N and interval logic


ACHTUNG!
I have to use next buffer for UART. I'm missing data.

*/