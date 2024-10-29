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


// Shortcuts for acquisition modes
void start_continuous_acq(const DataPacket* data);
void start_stroboscopic_acq(const DataPacket* data);
void start_ALEX_acq(const DataPacket* data);