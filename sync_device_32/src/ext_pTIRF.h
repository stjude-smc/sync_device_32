/*
 * ext_pTIRF.h
 *
 * Extensions (shortcuts) for pTIRF instrument
 *
 * Created: 10/28/2024 9:19:40 PM
 *  Author: rkiselev
 */ 


#pragma once
#include "globals.h"
#include "uart_comm.h"

// Shortcuts for shutter control
void open_shutters(uint32_t mask = 0);
void close_shutters(uint32_t mask = 0);

void select_lasers(uint32_t mask);
uint32_t selected_lasers();

// Additional scheduling functions
void schedule_shutter_pulse(uint32_t pulse_duration_us, uint64_t timestamp_us, uint32_t N, uint32_t interval_us, bool relative);
void open_shutters_func(uint32_t mask, uint32_t);
void close_shutters_func(uint32_t mask, uint32_t);

// Shortcuts for acquisition modes
void start_continuous_acq(const DataPacket* data);
void start_stroboscopic_acq(const DataPacket* data);
void start_ALEX_acq(const DataPacket* data);