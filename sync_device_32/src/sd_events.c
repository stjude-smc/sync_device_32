/*
 * sd_events.c
 *
 * Created: 9/9/2024 5:09:35 PM
 *  Author: rkiselev
 */ 

#include <asf.h>
#include "sd_events.h"
#include "sd_triggers.h"
#include "sd_comport.h"

Pulse pulse_table[10] = {0};

void f1(void){set_lasers(0b0000);}
void f2(void){set_lasers(0b0010);}
void f3(void){set_lasers(0b0110);}
void f4(void){set_lasers(0b0010);}
void f5(void){set_lasers(0b1100);}
void f6(void){set_lasers(0b1111);}
void f7(void){set_lasers(0b0110);}
void f8(void){set_lasers(0b1110);}
void f9(void){set_lasers(0b0110);}
void f10(void){set_lasers(0b1010);}

// Define a structure for timestamp and corresponding function pointer
typedef struct {
	uint32_t timestamp;   // Time in counts
	void (*func)(void);   // Function to call when time is reached
} PulseTrainEntry;

#define num_entries 20

PulseTrainEntry pulseTrainTable[num_entries] = {
	{ 100000, f1 },
	{ 200000, f2 },
	{ 300000, f3 },
	{ 400000, f4 },
	{ 500000, f5 },
	{ 600000, f6 },
	{ 600001, f7 },
	{ 800000, f8 },
	{ 900000, f9 },
	{ 1000000, f10 },
	{ 1500000, f5 },
	{ 1600000, f4 },
	{ 1700000, f7 },
	{ 1800000, f8 },
	{ 1900000, f9 },
	{ 2100000, f10 },
	{ 2200000, f1 },
	{ 2300000, f2 },
	{ 2400000, f3 },
	{ 2500000, f4 }
};

void start_ote_timer(void)
{
	sysclk_enable_peripheral_clock(ID_OTE_TC);
	
	tc_init(OTE_TC, OTE_TC_CH,
			TC_CMR_TCCLKS_TIMER_CLOCK4 | TC_CMR_WAVE
	);
	
	tc_write_ra(OTE_TC, OTE_TC_CH, pulseTrainTable[0].timestamp);
	// Enable the interrupt on register compare
	tc_enable_interrupt(OTE_TC, OTE_TC_CH, TC_IER_CPAS);
	
	NVIC_EnableIRQ(OTE_IRQn);
	
	tc_start(OTE_TC, OTE_TC_CH);
}

// Index of the next event to trigger
static uint32_t next_event_index = 0;

void OTE_Handler(void)
{
	tc_get_status(OTE_TC, OTE_TC_CH);
	return;
    // Read Timer Counter Status to clear the interrupt flag
    if (tc_get_status(OTE_TC, OTE_TC_CH) & TC_SR_CPAS) {
	    // Get the current time (microseconds)
	    uint32_t currentTime = tc_read_cv(OTE_TC, OTE_TC_CH);

	    // Handle back-to-back events by processing all events whose timestamps have passed
	    while (next_event_index < num_entries && currentTime >= pulseTrainTable[next_event_index].timestamp) {
		    // Call the corresponding function
		    pulseTrainTable[next_event_index].func();

		    // Move to the next event in the table
		    next_event_index++;

		    // Update the current time in case multiple events are very close together
		    currentTime = tc_read_cv(OTE_TC, OTE_TC_CH);
	    }

	    // If more events are left, set the next compare match
	    if (next_event_index < num_entries) {
			tc_write_ra(OTE_TC, OTE_TC_CH, pulseTrainTable[next_event_index].timestamp);
	    }
    }
	
	if (next_event_index >= num_entries)
	{
		// start over
		next_event_index = 0;
		tc_write_ra(OTE_TC, OTE_TC_CH, pulseTrainTable[0].timestamp);
		tc_start(OTE_TC, OTE_TC_CH);
	}
}
