#pragma once

#include "globals.h"
#include "uart_comm.h"
#include "events.h"

// System Properties for GET command
enum SysProps {
	prop_VERSION,
	prop_SYS_TIMER_STATUS,
	prop_SYS_TIMER_VALUE,
	prop_SYS_TIMER_OVF_COUNT,
	prop_SYS_TIME_S,
	prop_SYS_TIMER_PRESCALER,
	prop_DFLT_PULSE_DURATION_US,
	prop_WATCHDOG_TIMEOUT,
	prop_N_EVENTS
};


void get_property(SysProps prop);
void set_property(SysProps prop, uint32_t value);