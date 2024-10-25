/*
 * pins.h
 *
 * Created: 9/19/2024 12:11:24 PM
 *  Author: rkiselev
 */ 

#pragma once

#include <asf.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Pin mapping for Arduino Due (Digital and Analog pins)
uint32_t pin_name_to_ioport_id(const uint32_t pin_name);
uint32_t pin_name_to_ioport_id(const char* pin_name);

typedef struct {
	const char *pin_name;
	uint32_t pin_idx;
} pin_map_t;

// pin_map array
extern const pin_map_t pin_map[];

class Pin {
private:
	bool level;
	bool active;

public:
	uint32_t pin_idx;
	
	Pin() : level(false), active(true) {};
	
	void set_level(bool level);
	void toggle();
	void enable();
	void disable();
};

extern Pin pins[107];

void init_pins();