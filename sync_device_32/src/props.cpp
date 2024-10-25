/*
 * props.cpp
 *
 * Get/set system properties
 *
 * Created: 10/16/2024 2:59:57 PM
 *  Author: rkiselev
 */ 

#include "props.h"


void get_property(SysProps prop)
{
	switch (prop)
	{
	case prop_VERSION:
		printf("%s\n", VERSION);
		break;
	case prop_SYS_TIMER_STATUS:
		printf("%s\n", sys_timer_running ? "RUNNING" : "STOPPED");
		break;
	case prop_SYS_TIMER_VALUE:
		printf("%lu\n", SYS_TC->TC_CHANNEL[SYS_TC_CH].TC_CV);
		break;
	case prop_SYS_TIMER_OVF_COUNT:
		printf("%lu\n", (uint32_t) (sys_tc_ovf_count >> 32));
		break;
	case prop_SYS_TIME_S:
		printf("%f\n", current_time_s());
		break;
	case prop_SYS_TIMER_PRESCALER:
		printf("%lu\n", SYS_TC_PRESCALER);
		break;
	case prop_DFLT_PULSE_DURATION_US:
		printf("%lu\n", DFLT_PULSE_DURATION);
		break;
	case prop_WATCHDOG_TIMEOUT:
		printf("%lu\n", WATCHDOG_TIMEOUT);
		break;
	case prop_N_EVENTS:
		printf("%i\n", event_queue.size());
		break;

	default:
		printf("ERR: unknown property with id=%lu\n", (uint32_t) prop);
	}
}

void set_property(SysProps prop, uint32_t value)
{
	printf("ERR: set_property() is not implemented\n");
}
