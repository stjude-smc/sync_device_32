#pragma once

#include "globals.h"


// System Properties
enum SysProps {
	ro_VERSION,
	ro_SYS_TIMER_STATUS,
	ro_SYS_TIMER_VALUE,
	ro_SYS_TIMER_OVF_COUNT,
	ro_SYS_TIME_ms,
	ro_SYS_TIMER_PRESCALER,
	rw_DFLT_PULSE_DURATION_us,
	ro_WATCHDOG_TIMEOUT_ms,
	ro_N_EVENTS,
	rw_INTLCK_ENABLED,

	// pTIRF extension
	rw_SELECTED_LASERS,
	wo_OPEN_SHUTTERS,
	wo_CLOSE_SHUTTERS,
	rw_SHUTTER_DELAY_us,
	rw_CAM_READOUT_us
};

enum class PropertyAccess {
	ReadOnly,
	ReadWrite,
	WriteOnly
};

using PropGetter = uint32_t (*)();
using PropSetter = void (*)(uint32_t);

class DeviceProperty {
public:
	explicit DeviceProperty(PropertyAccess access) : access(access) {}
	virtual ~DeviceProperty() = default;

	virtual uint32_t get_value() const = 0;
	virtual void set_value(uint32_t newValue) = 0;

protected:
	PropertyAccess access;
};

class InternalProperty : public DeviceProperty {
private:
	uint32_t value;

public:
	InternalProperty(uint32_t value, PropertyAccess access = PropertyAccess::ReadOnly);

	uint32_t get_value() const override;
	void set_value(uint32_t new_value) override;
};

class ExternalProperty : public DeviceProperty {
private:
	uint32_t* externalValue;

public:
	ExternalProperty(uint32_t* externalValue, PropertyAccess access = PropertyAccess::ReadOnly);

	uint32_t get_value() const override;
	void set_value(uint32_t new_value) override;
};

class FunctionProperty : public DeviceProperty {
private:
	PropGetter getter;
	PropSetter setter;

public:
	FunctionProperty(PropGetter getter, PropSetter setter, PropertyAccess access = PropertyAccess::ReadOnly);

	uint32_t get_value() const override;
	void set_value(uint32_t new_value) override;
};



void init_props();
uint32_t get_property(SysProps prop);
void set_property(SysProps prop, uint32_t value);
