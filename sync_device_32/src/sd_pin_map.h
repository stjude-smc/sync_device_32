/*
 * sd_pin_map.h
 *
 * Created: 9/12/2024 4:36:32 PM
 *  Author: rkiselev
 */ 

#pragma once

#include <asf.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Function prototype
uint32_t pin_name_to_ioport_id(uint8_t *pin_name);

// Pin mapping for Arduino Due (Digital and Analog pins)
typedef struct {
	const char *pin_name;
	uint32_t pin_idx;
} pin_map_t;

// pin_map array
extern const pin_map_t pin_map[];
