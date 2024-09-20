/*
 * sd_events.c
 *
 * Created: 9/9/2024 5:09:35 PM
 *  Author: rkiselev
 */ 

#include <asf.h>
#include "events.h"
#include "pins.h"

// Create a table of events
std::priority_queue<Event> event_queue;
    
Event event_table_OLD[MAX_N_EVENTS] = {0};
volatile uint32_t event_table_start = 0;  // Pointer to the next pending event
volatile uint32_t event_table_end = 0;    // Pointer to the last pending event

/************************************************************************/
/*                  INTERNAL FUNCTION PROTOTYPES                        */
/************************************************************************/
void _toggle_pin_func(uint32_t pin_idx, uint32_t arg2);
void _set_pin_func(uint32_t pin_idx, uint32_t arg2);
static inline void _enable_event_irq();
static inline void _disable_event_irq();

/************************************************************************/
/*                 EVENT HANDLING AND PROCESSING                        */
/************************************************************************/

// Populate an Event with data from a DataPacket. You still have to assign a function!
void event_from_datapacket(const DataPacket* packet, Event* new_event)
{
	// Copy fields from DataPacket to Event
	new_event->arg1 = packet->arg1;
	new_event->arg2 = packet->arg2;
	new_event->timestamp = packet->timestamp;
	
	// N is 1 (one-time event) if interval is too small
	new_event->N = (packet->interval < MIN_EVENT_INTERVAL) ? 1 : packet->N;
	new_event->interval = packet->interval;
}


// Schedule event relative to t=0
void schedule_event_abs_time(Event event)
{
	// Do we have enough memory?
	if (event_queue.size() >= MAX_N_EVENTS)
	{
		sd_tx("ERR: event table is full!\n");
		return;
	}

	event.timestamp = us2cts(event.timestamp + UNIFORM_TIME_DELAY);

	// Lock the event table and add a new event. Update RA register
	_disable_event_irq();
		event_queue.push(event);
	
		// grab the next event
		Event retrieved_event = event_queue.top();
		// Update the RA register for compare interrupt
		tc_write_ra(SYS_TC, SYS_TC_CH, retrieved_event.timestamp);

		if (is_sys_timer_running() && current_time_cts() > tc_read_ra(SYS_TC, SYS_TC_CH))
		{
			// we must have missed an event while fucking with the event_queue
			process_events();  // <- internally updates RA
		}
	_enable_event_irq();
}


// Schedule event relative to the current time
void schedule_event(Event event)
{
	event.timestamp += current_time_us();
	printf(" actual timestamp is %lu\n", event.timestamp);
	schedule_event_abs_time(event);
}


void schedule_pulse(DataPacket data)
{
	// Activate this output pin
	data.arg1 = pin_name_to_ioport_id(data.arg1);
	bool lvl = ioport_get_pin_level(data.arg1);

	ioport_set_pin_dir(data.arg1, IOPORT_DIR_OUTPUT);

	Event event;
	event_from_datapacket(&data, &event);
	event.arg2 = lvl ? 0 : 1;
	event.func = _set_pin_func;

	// Schedule front of the pulse
	event.timestamp += current_time_us();
	uint32_t t1 = event.timestamp;
	uint32_t l1 = event.arg2;
	schedule_event_abs_time(event);

	// Schedule back of the pulse. arg2 is the pulse duration		
	event.timestamp += (data.arg2 > 0) ? data.arg2 : DFL_PULSE_DURATION;
	event.arg2 = lvl ? 1 : 0;
	schedule_event_abs_time(event);
	printf("Event 1 lvl=%d at t=%lu\n", l1, t1);
	printf("Event 2 lvl=%d at t=%lu\n", event.arg2, event.timestamp);
}


void schedule_pin(DataPacket data)
{
	// Activate this output pin
	data.arg1 = pin_name_to_ioport_id(data.arg1);
	ioport_set_pin_dir(data.arg1, IOPORT_DIR_OUTPUT);

	Event event;
	event_from_datapacket(&data, &event);
	event.func = _set_pin_func;

	printf("PIN: requested timestamp = %lu us", event.timestamp);

	schedule_event(event);
}

void schedule_toggle(DataPacket data)
{
	// Activate this output pin
	data.arg1 = pin_name_to_ioport_id(data.arg1);
	ioport_set_pin_dir(data.arg1, IOPORT_DIR_OUTPUT);

	Event event;
	event_from_datapacket(&data, &event);
	event.func = _toggle_pin_func;

	schedule_event(event);

}



/************************************************************************/
/*                       SYSTEM TIMER CONTROL                           */
/************************************************************************/

bool is_sys_timer_running()
{
	return tc_get_status(SYS_TC, SYS_TC_CH) & TC_SR_CLKSTA;
}

// return current system time in counts
uint32_t current_time_cts()
{
	return is_sys_timer_running() ? tc_read_cv(SYS_TC, SYS_TC_CH) : 0;
}

// return current system time in microseconds
uint32_t current_time_us()
{
	return cts2us(current_time_cts());
}

// Start timer from 0
void start_sys_timer()
{
	tc_start(SYS_TC, SYS_TC_CH);
}

void init_sys_timer()
{
	sysclk_enable_peripheral_clock(ID_SYS_TC);
	
	tc_init(SYS_TC, SYS_TC_CH,
			TC_CMR_TCCLKS_TIMER_CLOCK3 | TC_CMR_WAVE
	);
	
	// Enable the interrupt on register compare
	_enable_event_irq();
	
	NVIC_EnableIRQ(SYS_TC_IRQn);
}

// This re-adjusts timestamps of scheduled events so we can pause and continue
void pause_sys_timer()
{
	tc_stop(SYS_TC, SYS_TC_CH);
	//uint32_t stopped_at = tc_read_cv(SYS_TC, SYS_TC_CH);
	
	// update all pending events in the event table
	printf("ERR: pause_sys_timer() - NOT IMPLEMENTED");
}


/************************************************************************/
/*                  SYSTEM TIMER INTERRUPTS                             */
/************************************************************************/

// This interrupt runs when current time reaches the timestamp of the next
// scheduled event in the event_queue. We might end up processing more than
// one event.
void SYS_TC_Handler()
{
    // Read Timer Counter Status to clear the interrupt flag
	uint32_t status = tc_get_status(SYS_TC, SYS_TC_CH);
	
    if (status & TC_SR_CPAS) {  // RA match
		process_events();
    }


	// TODO - Overflow interrupt?
}



/************************************************************************/
/*                           INTERNAL FUNCTIONS                         */
/************************************************************************/

void process_events()
{
	_disable_event_irq();
	Event event;
	while (!event_queue.empty()) {
		// Keep processing events from the queue while they are pending
		event = event_queue.top();		if (current_time_cts() >= event.timestamp)		{			// Fire the event function			event.func(event.arg1, event.arg2);			event_queue.pop();  // remove the event from the queue							// Process the event metadata			if (event.interval > 0) // repeating event			{				event.timestamp += us2cts(event.interval);				if (event.N == 0){  // infinite event					event_queue.push(event);				}				// if N == 1, it was a last call, and we drop it				if (event.N > 1) {  // reschedule the event					event.N--;					event_queue.push(event);				}			}		}		else  // This is a future event		{			tc_write_ra(SYS_TC, SYS_TC_CH, event.timestamp);			break;		}	}
	_enable_event_irq();
}

static inline void _disable_event_irq()
{
	tc_disable_interrupt(SYS_TC, SYS_TC_CH, TC_IER_CPAS);
}
static inline void _enable_event_irq()
{
	tc_enable_interrupt(SYS_TC, SYS_TC_CH, TC_IER_CPAS);
}


void _toggle_pin_func(uint32_t pin_idx, uint32_t arg2)
{
	ioport_toggle_pin_level(pin_idx);
}

void _set_pin_func(uint32_t pin_idx, uint32_t arg2)
{
	ioport_set_pin_level(pin_idx, arg2);
}