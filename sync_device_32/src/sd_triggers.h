/**
 * @file triggers.h
 * @author Roman Kiselev (roman.kiselev@stjude.org)
 * @brief Functions to read/write IO ports and generate trigger signals
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include "sd_globals.h"
#include "sd_comport.h"

// Initialize all I/O ports
void sd_init_IO(void);

// Write data to I/O port that controls laser shutters
void set_lasers(uint8_t laser);

// Close all laser shutters
void lasers_off(void);
