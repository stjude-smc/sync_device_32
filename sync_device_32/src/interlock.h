/*
 * interlock.h
 *
 * Created: 10/23/2024 5:08:09 PM
 *  Author: rkiselev
 */ 

#pragma once

void init_interlock();

extern volatile bool lasers_enabled;
extern bool interlock_enabled;
