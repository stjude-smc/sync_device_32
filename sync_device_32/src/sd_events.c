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

// Define a structure for timestamp and corresponding function pointer
typedef struct {
	uint32_t timestamp;   // Time in counts
	void (*func)(void);   // Function to call when time is reached
} PulseTrainEntry;

#define num_entries 20


void start_ote_timer(void)
{
	sysclk_enable_peripheral_clock(ID_OTE_TC);
	
	tc_init(OTE_TC, OTE_TC_CH,
			TC_CMR_TCCLKS_TIMER_CLOCK4 | TC_CMR_WAVE
	);
	
	// tc_write_ra(TC1, 0, pulseTrainTable[0].timestamp);  // FIXME
	// Enable the interrupt on register compare
	tc_enable_interrupt(OTE_TC, OTE_TC_CH, TC_IER_CPAS);
	
	NVIC_EnableIRQ(OTE_IRQn);
	
	tc_start(OTE_TC, OTE_TC_CH);
}

// Index of the next event to trigger
//static uint32_t next_event_index = 0;


// FIXME
/*
void TC3_Handler(void)
{
    // Read Timer Counter Status to clear the interrupt flag
    if (TC1->TC_CHANNEL[0].TC_SR & TC_SR_CPAS) {
	    // Get the current time (microseconds)
	    uint32_t currentTime = TC1->TC_CHANNEL[0].TC_CV;

	    // Handle back-to-back events by processing all events whose timestamps have passed
	    while (next_event_index < num_entries && currentTime >= pulseTrainTable[next_event_index].timestamp) {
		    // Call the corresponding function
		    pulseTrainTable[next_event_index].func();

		    // Move to the next event in the table
		    next_event_index++;

		    // Update the current time in case multiple events are very close together
		    currentTime = TC1->TC_CHANNEL[0].TC_CV;
	    }

	    // If more events are left, set the next compare match
	    if (next_event_index < num_entries) {
		    TC1->TC_CHANNEL[0].TC_RA = pulseTrainTable[next_event_index].timestamp;
	    }
    }
	
	if (next_event_index >= num_entries)
	{
		// start over
		next_event_index = 0;
		tc_write_ra(TC1, 0, pulseTrainTable[0].timestamp);
		tc_start(TC1, 0);
	}
}
*/