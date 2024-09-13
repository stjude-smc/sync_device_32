#include "sd_triggers.h"
#include <stdlib.h>

// Prototypes of local functions
uint8_t get_lowest_bit(uint8_t v);
uint8_t lowest_bit_position(uint8_t v);
uint8_t highest_bit_position(uint8_t v);


// Configure output ports (data direction registers and default values)
void sd_init_IO(void)
{
	// Initialize the IOPORT system
	ioport_init();   // Initialize the IOPORT system

	// Configure the laser shutters as outputs
	ioport_set_port_dir(SHUTTERS_PORT, SHUTTERS_MASK, IOPORT_DIR_OUTPUT);

	// Configure the fluidic trigger as output
	ioport_set_pin_dir(FLUIDIC_PIN, IOPORT_DIR_OUTPUT);

	// Configure the camera trigger as output
	ioport_set_pin_dir(CAMERA_PIN, IOPORT_DIR_OUTPUT);
}


// Bit 0: Cy2, bit 3: Cy7
void set_lasers(uint8_t laser)
{
	uint32_t cy2 = laser & 1 ? ioport_pin_to_mask(CY2_PIN) : 0;
	uint32_t cy3 = laser & 2 ? ioport_pin_to_mask(CY3_PIN) : 0;
	uint32_t cy5 = laser & 4 ? ioport_pin_to_mask(CY5_PIN) : 0;
	uint32_t cy7 = laser & 8 ? ioport_pin_to_mask(CY7_PIN) : 0;
	
	uint32_t active_pins = cy2 | cy3 | cy5 | cy7;
	ioport_set_port_level(SHUTTERS_PORT, SHUTTERS_MASK & active_pins, 1);  // activate pins
	ioport_set_port_level(SHUTTERS_PORT, SHUTTERS_MASK & ~active_pins, 0); // deactivate pins
}

void lasers_off(void)
{
	ioport_set_port_level(SHUTTERS_PORT, SHUTTERS_MASK, 0);
}


uint8_t get_lowest_bit(uint8_t v)
{
	return v & -v;
}

uint8_t lowest_bit_position(uint8_t v)
{
	uint8_t r = 0;
	uint8_t lowest_bit = get_lowest_bit(v);
	while (lowest_bit >>= 1)
	{
		r++;
	}
	return r;
}

uint8_t highest_bit_position(uint8_t v)
{
	uint8_t r = 0;
	while (v >>= 1)
	{
		r++;
	}
	return r;
}

/*
uint8_t next_laser()
{
	uint8_t laser_id = lowest_bit_position(sys.current_laser);
	uint8_t next_active = 0;
	
	while(1)
	{
		laser_id++;
		next_active = (sys.lasers_in_use >> laser_id) & 1UL;
		if (next_active)
		{
			return 1UL << laser_id;
		}
		if (laser_id == 8)
		{
			return get_lowest_bit(sys.lasers_in_use);
		}
	}
}

void reset_lasers()
{
	if (sys.ALEX_enabled)
	{
		sys.current_laser = get_first_laser();
	}
	else
	{
		sys.current_laser = sys.lasers_in_use;
	}
}

uint8_t get_first_laser()
{
	return get_lowest_bit(sys.lasers_in_use);
}
*/