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
size_t pulse_table_n_items = 0;

int pulse_srt_pending_desc(const Pulse* p1, const Pulse* p2);
int pulse_srt_ts_asc(const Pulse* p1, const Pulse* p2);

// Comparison functions
int pulse_srt_pending_desc(const Pulse* p1, const Pulse* p2) {
	return p2->pending - p1->pending;
}

int pulse_srt_ts_asc(const Pulse* p1, const Pulse* p2) {
	return p1->timestamp - p2->timestamp;
}

// this takes about 25us
void update_pulse_table(void)
{
	// update size of the pulse_table
	qsort(pulse_table, 10, sizeof(pulse_table[0]), pulse_srt_pending_desc);
	for (int j = 0; j<=10; j++)
	{
		if (!(pulse_table[j].pending))
		{
			pulse_table_n_items = j;  // update number of elements
			break;
		}
	}
	// sort by timestamp
	qsort(pulse_table, pulse_table_n_items, sizeof(pulse_table[0]), pulse_srt_ts_asc);
}

void send_pulse(ioport_pin_t pin, uint32_t duration)
{
	ioport_toggle_pin_level(pin);
	Pulse* p_pulse = &pulse_table[pulse_table_n_items];
	p_pulse->pending = true;
	p_pulse->pin = pin;
	p_pulse->timestamp = tc_read_cv(OTE_TC, OTE_TC_CH) + duration;
	update_pulse_table();
}

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