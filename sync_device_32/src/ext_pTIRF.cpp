/*
 * ext_pTIRF.cpp
 *
 * Created: 10/28/2024 9:25:32 PM
 *  Author: rkiselev
 */ 

#include "ext_pTIRF.h"
#include "events.h"
#include "props.h"

/************************************************************************/
/*                 HELPER FUNCTIONS                                     */
/************************************************************************/

int _count_set_bits(unsigned int bitmask) {
	int count = 0;
	while (bitmask) {
		count += bitmask & 1; // Increment count if the last bit is set
		bitmask >>= 1;        // Right shift the bitmask by 1
	}
	return count;
}

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

void schedule_shutter_pulse(uint32_t pulse_duration_us,
                            uint64_t timestamp_us, uint32_t N, uint32_t interval_us,
							bool relative)
{
	uint64_t now_cts = (relative && sys_timer_running) ? current_time_cts() : 0;
	
	Event event;
	event.func = open_shutters_func;
	event.arg1 = selected_lasers();
	event.ts64_cts = us2cts(timestamp_us) + now_cts;
	event.N = N;
	event.interv_cts = us2cts(interval_us);
	schedule_event(&event, false);
	
	event.func = close_shutters_func;
	event.ts64_cts += us2cts(pulse_duration_us);
	schedule_event(&event, false);
}

// Functions that can be used within even queue
void open_shutters_func(uint32_t mask, uint32_t){open_shutters(mask);}
void close_shutters_func(uint32_t mask, uint32_t){close_shutters(mask);}


/************************************************************************/
/*              SHORTCUTS FOR ACQUISITION MODES                         */
/************************************************************************/

// arg1 - exposure_time_us
// arg2 - ignored
// ts - when to start imaging (if not now)
// N - number of frames
// interval - ignored
void start_continuous_acq(const DataPacket* data)
{
	uint32_t exp_us = data->arg1;
	uint32_t camera_us  = get_property(rw_CAM_READOUT_us);
	uint32_t shutter_us = get_property(rw_SHUTTER_DELAY_us);
	uint64_t acq_start_us = current_time_us() + data->ts_us + UNIFORM_TIME_DELAY;

	// Schedule shutters (once, no repeats, absolute time)
	schedule_shutter_pulse(
		data->N * exp_us + shutter_us,			// duration
		acq_start_us + camera_us - shutter_us,	// timestamp
		1, 0, false);
	
	// Schedule a single dummy camera pulse to readout the sensor
	schedule_pulse(CAMERA_PIN, default_pulse_duration_us, acq_start_us, 1, 0, false);
	
	// Schedule pulse train for the camera (N+1 times, after camera readout)
	schedule_pulse(CAMERA_PIN, default_pulse_duration_us, acq_start_us + camera_us,
	               data->N+1, exp_us, false);
}

// arg1 - exposure_time_us
// arg2 - ignored
// ts - when to start imaging (if not now)
// N - number of frames
// interval - frame period
void start_stroboscopic_acq(const DataPacket* data)
{
	uint32_t exp_us = data->arg1;
	uint32_t camera_us  = get_property(rw_CAM_READOUT_us);
	uint32_t shutter_us = get_property(rw_SHUTTER_DELAY_us);
	uint64_t acq_start_us = current_time_us() + data->ts_us + UNIFORM_TIME_DELAY;

	// make sure frame period isn't too short
	uint32_t frame_period_us = std::max(exp_us + camera_us + shutter_us, data->interv_us);
	
	// Schedule pulse train for shutters
	schedule_shutter_pulse(exp_us, acq_start_us, data->N, frame_period_us, false);
	
	// Schedule pulse train for the camera
	// TODO - do we need a dummy read??
	schedule_pulse(CAMERA_PIN, exp_us,
		acq_start_us + shutter_us,
		data->N, frame_period_us, false);
}

// arg1 - exposure_time_us
// arg2 - ignored
// ts - when to start imaging (if not now)
// N - number of frames
// interval - frame period
void start_ALEX_acq(const DataPacket* data)
{
	uint32_t exp_us = data->arg1;
	uint32_t camera_us  = get_property(rw_CAM_READOUT_us);
	uint32_t shutter_us = get_property(rw_SHUTTER_DELAY_us);
	uint64_t acq_start_us = current_time_us() + data->ts_us + UNIFORM_TIME_DELAY;

	uint32_t N_channels = _count_set_bits(get_property(rw_SELECTED_LASERS));
	
	// make sure burst period isn't too short
	uint32_t frame_duration_us = exp_us + camera_us + shutter_us;
	uint32_t burst_period_us = std::max(N_channels*frame_duration_us, data->interv_us);

	// Iterate through laser channels
	uint32_t frame_start_us = acq_start_us;
	for (uint32_t i = 0; i < 4; ++i)
	{
		if (pins[shutter_pins[i]].is_active()) // laser is enabled
		{
			// Schedule laser shutter pulse
			schedule_pulse(pins[shutter_pins[i]].pin_idx, exp_us, frame_start_us, data->N, burst_period_us, false);

			// Schedule camera pulse
			//  TODO - do we need a dummy read?
			schedule_pulse(CAMERA_PIN, exp_us, frame_start_us + shutter_us, data->N, burst_period_us, false);

			// Calculate the start time of the next frame
			frame_start_us += frame_duration_us;
		}
	}
}
