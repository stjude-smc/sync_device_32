/*
 * ext_pTIRF.cpp
 *
 * Created: 10/28/2024 9:25:32 PM
 *  Author: rkiselev
 */ 

#include "ext_pTIRF.h"

/************************************************************************/
/*                SHORTCUTS FOR SHUTTER CONTROL                         */
/************************************************************************/

void open_shutters(uint32_t mask)
{
	if (mask == 0)
	{
		mask = 0b1111;
	}
	for (uint32_t i = 0; i < 4; ++i)
	{
		if (mask & (1 << i))
		{
			pins[shutter_pins[i]].set_level(true);
		}
	}
}

void close_shutters(uint32_t mask)
{
	if (mask == 0)
	{
		mask = 0b1111;
	}
	for (uint32_t i = 0; i < 4; ++i)
	{
		if (mask & (1 << i))
		{
			pins[shutter_pins[i]].set_level(false);
		}
	}
}

void select_lasers(uint32_t mask)
{
	for (uint32_t i = 0; i < 4; ++i)
	{
		if (mask & (1 << i))
		{
			pins[shutter_pins[i]].enable();
		}
		else
		{
			pins[shutter_pins[i]].disable();
		}
	}
}

uint32_t selected_lasers()
{
	uint32_t mask = 0;
	for (uint32_t i = 0; i < 4; ++i)
	{
		if (pins[shutter_pins[i]].is_active())
		{
			mask |= (1 << i);
		}
	}
	return mask;
}


/************************************************************************/
/*              SHORTCUTS FOR ACQUISITION MODES                         */
/************************************************************************/

void start_continuous_acq(const DataPacket* data)
{
	// 
}
void start_stroboscopic_acq(const DataPacket* data){}
void start_ALEX_acq(const DataPacket* data){}