/*
 * props.cpp
 *
 * Get/set system properties
 *
 * Created: 10/16/2024 2:59:57 PM
 *  Author: rkiselev
 */ 

#include <unordered_map>

#include "props.h"

// Global map to store properties by their ID
std::unordered_map<SysProps, DeviceProperty*> props;


void print_version(){printf("%s\n", VERSION);}
void print_timer_cv(){printf("%lu\n", SYS_TC->TC_CHANNEL[SYS_TC_CH].TC_CV);}
void print_sys_tc_ovf(){printf("%lu\n", (uint32_t) (sys_tc_ovf_count >> 32));}
void print_time_s(){printf("%f\n", current_time_s());}
void print_N_events(){printf("%u\n", event_queue.size());}


void init_props() {
	props[prop_VERSION]                = new FunctionProperty(print_version);
	props[prop_SYS_TIMER_STATUS]       = new ExternalProperty((uint32_t*) &sys_timer_running);
	props[prop_SYS_TIMER_VALUE]        = new FunctionProperty(print_timer_cv);
	props[prop_SYS_TIMER_OVF_COUNT]    = new FunctionProperty(print_sys_tc_ovf);
	props[prop_SYS_TIME_s]             = new FunctionProperty(print_time_s);
	props[prop_SYS_TIMER_PRESCALER]    = new InternalProperty((uint32_t) SYS_TC_PRESCALER);
	props[prop_DFLT_PULSE_DURATION_us] = new InternalProperty((uint32_t) DFLT_PULSE_DURATION);
	props[prop_WATCHDOG_TIMEOUT_ms]    = new InternalProperty((uint32_t) WATCHDOG_TIMEOUT);
	props[prop_N_EVENTS]               = new FunctionProperty(print_N_events);
	props[prop_INTLCK_ACTIVE]          = new InternalProperty(1, PropertyAccess::ReadWrite);
}



void print_property(SysProps id)
{
	auto iterator = props.find(id);
	if (iterator == props.end())
	{
		printf("ERR: Property not found (ID: %d)\n", id);
		return;
	}
	iterator->second->print_value();
}


void set_property(SysProps id, uint32_t value)
{
    auto iterator = props.find(id);
    if (iterator == props.end()) {
        printf("ERR: Property not found (ID: %d)\n", id);
        return;
    }
    iterator->second->set_value(value);
}



void InternalProperty::print_value() const 
{
	printf("%lu\n", value);
}

void InternalProperty::set_value(uint32_t newValue)
{
	if (access == PropertyAccess::ReadOnly) {
		std::printf("ERR: read-only property\n");
		return;
	}
	value = newValue;
}

void ExternalProperty::print_value() const 
{
	printf("%lu\n", *externalValue);
}

void ExternalProperty::set_value(uint32_t newValue)
{
    if (access == PropertyAccess::ReadOnly) {
        std::printf("ERR: read-only property\n");
        return;
    }
    *externalValue = newValue;
}

void FunctionProperty::print_value() const 
{
	this->valueFunction();
}

void FunctionProperty::set_value(uint32_t)
{
	std::printf("ERR: read-only property\n");
}
