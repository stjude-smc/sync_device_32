#pragma once

#include "globals.h"


// System Properties
enum SysProps {
	ro_VERSION,
	ro_SYS_TIMER_STATUS,
	ro_SYS_TIMER_VALUE,
	ro_SYS_TIMER_OVF_COUNT,
	ro_SYS_TIME_s,
	ro_SYS_TIMER_PRESCALER,
	ro_DFLT_PULSE_DURATION_us,
	ro_WATCHDOG_TIMEOUT_ms,
	ro_N_EVENTS,
	rw_INTLCK_ENABLED,
};

enum class PropertyAccess {
	ReadOnly,
	ReadWrite
};

using PropFunc = void (*)();

// Abstract base class
class DeviceProperty {
public:
    virtual ~DeviceProperty() = default;
    virtual void print_value() const = 0;
	virtual uint32_t get_value() const = 0;
    virtual void set_value(uint32_t newValue) = 0;
};


// Internal property class
class InternalProperty : public DeviceProperty {
private:
    uint32_t value;
	PropertyAccess access;

public:
    InternalProperty(uint32_t value, PropertyAccess access = PropertyAccess::ReadOnly)
        : value(value), access(access) {}

    void print_value() const override;
	uint32_t get_value() const override;
    void set_value(uint32_t newValue) override;
};


// External property class
class ExternalProperty : public DeviceProperty {
private:
    uint32_t* externalValue;
	PropertyAccess access;

public:
    ExternalProperty(uint32_t* externalValue, PropertyAccess access = PropertyAccess::ReadOnly)
        : externalValue(externalValue), access(access) {}

    void print_value() const override;
	uint32_t get_value() const override;
    void set_value(uint32_t newValue) override;
};

// Function property class
class FunctionProperty : public DeviceProperty {
private:
    PropFunc valueFunction;

public:
    FunctionProperty(PropFunc func)
        : valueFunction(func) {}

    void print_value() const override;
	uint32_t get_value() const override;
    void set_value(uint32_t) override;
};




void init_props();
void print_property(SysProps prop);
void set_property(SysProps prop, uint32_t value);
