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
	uint64_t acq_start_us = current_time_us() + data->ts_us;

	uint32_t cam_readout_cts = us2cts(get_property(rw_CAM_READOUT_us));
	uint32_t shutter_delay_cts = us2cts(get_property(rw_SHUTTER_DELAY_us));

	// Schedule shutters
	Event event_p;
	event_p.func = open_shutters_func;
	event_p.arg1 = selected_lasers();
	event_p.ts64_cts = us2cts(acq_start_us) + cam_readout_cts - shutter_delay_cts;
	schedule_event(&event_p, false);  // open
	
	event_p.func = close_shutters_func;
	event_p.ts64_cts += data->N * us2cts(data->arg1) + shutter_delay_cts;
	schedule_event(&event_p, false);  // close
	
	// Schedule dummy camera pulse to readout the sensor
	schedule_pulse(CAMERA_PIN, default_pulse_duration_us,
				   // once, at requested timestamp
	               acq_start_us, 1, 0, false);
	
	// Schedule pulse train for the camera
	schedule_pulse(CAMERA_PIN, default_pulse_duration_us,
	               // N+1 times, after camera readout
	               acq_start_us + get_property(rw_CAM_READOUT_us),
				   data->N+1, data->arg1, false);
}

// arg1 - exposure_time_us
// arg2 - ignored
// ts - when to start imaging (if not now)
// N - number of frames
// interval - frame period
void start_stroboscopic_acq(const DataPacket* data)
{
	uint32_t exp_time_cts = us2cts(data->arg1);
	uint32_t now_cts = (sys_timer_running) ? current_time_cts() : 0;
	uint32_t ts_cts = us2cts(data->ts_us);
	uint32_t cam_readout_cts = us2cts(get_property(rw_CAM_READOUT_us));
	uint32_t shutter_delay_cts = us2cts(get_property(rw_SHUTTER_DELAY_us));
	uint32_t frame_period_cts = us2cts(data->interv_us);
	
	// make sure frame period isn't too short
	frame_period_cts = std::max(exp_time_cts + cam_readout_cts + shutter_delay_cts, frame_period_cts);

	Event* event_p = event_from_datapacket(data, open_shutters_func);
	event_p->interv_cts = frame_period_cts;
	event_p->N = data->N;
	// Schedule shutters
	event_p->ts64_cts += now_cts;
	event_p->arg1 = selected_lasers();
	schedule_event(event_p, false); // open
	
	event_p->func = close_shutters_func;
	event_p->ts64_cts += exp_time_cts;
	schedule_event(event_p, false); // close


	// Schedule pulse train for the camera
	//  TODO - do we need a dummy read?
	event_p->func = set_pin_event_func;
	event_p->arg1 = CAMERA_PIN;
	event_p->arg2 = 1;
	event_p->ts64_cts = ts_cts + now_cts + shutter_delay_cts;
	schedule_event(event_p, false); // front

	event_p->arg2 = 0;
	event_p->ts64_cts += exp_time_cts;
	schedule_event(event_p, false); // back
	
	delete event_p;
}

// arg1 - exposure_time_us
// arg2 - ignored
// ts - when to start imaging (if not now)
// N - number of frames
// interval - frame period
void start_ALEX_acq(const DataPacket* data)
{
	uint32_t N_channels = _count_set_bits(get_property(rw_SELECTED_LASERS));
	
	uint32_t exp_time_cts = us2cts(data->arg1);
	uint32_t now_cts = (sys_timer_running) ? current_time_cts() : 0;
	uint32_t ts_cts = us2cts(data->ts_us);
	uint32_t cam_readout_cts = us2cts(get_property(rw_CAM_READOUT_us));
	uint32_t shutter_delay_cts = us2cts(get_property(rw_SHUTTER_DELAY_us));
	uint32_t burst_period_cts = us2cts(data->interv_us);
	
	uint32_t frame_duration_cts = exp_time_cts + cam_readout_cts + shutter_delay_cts;
	
	// make sure burst period isn't too short
	burst_period_cts = std::max(N_channels*frame_duration_cts, burst_period_cts);

	Event* event_p = new Event;
	event_p->func = set_pin_event_func;
	event_p->interv_cts = burst_period_cts;
	event_p->N = data->N;
	
	// Iterate through laser channels
	uint32_t frame_start_cts = ts_cts + now_cts;
	for (uint32_t i = 0; i < 4; ++i)
	{
		if (pins[shutter_pins[i]].is_active()) // laser is enabled
		{
			// Schedule shutters
			event_p->ts64_cts = frame_start_cts;
			event_p->arg1 = pins[shutter_pins[i]].pin_idx;
			event_p->arg2 = 1;
			schedule_event(event_p, false); // open
	
			event_p->arg2 = 0;
			event_p->ts64_cts += exp_time_cts;
			schedule_event(event_p, false); // close


			// Schedule pulse train for the camera
			//  TODO - do we need a dummy read?
			event_p->arg1 = CAMERA_PIN;
			event_p->arg2 = 1;
			event_p->ts64_cts = frame_start_cts + shutter_delay_cts;
			schedule_event(event_p, false); // front

			event_p->arg2 = 0;
			event_p->ts64_cts += exp_time_cts;
			schedule_event(event_p, false); // back

			// Calculate start timepoint of the next frame
			frame_start_cts += frame_duration_cts;
		}
	}

	delete event_p;
}
