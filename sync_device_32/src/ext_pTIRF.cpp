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

struct AcqParams {
	uint32_t exp;
	uint32_t cam;
	uint32_t shutter;
	uint64_t start;

	AcqParams(const DataPacket* data) {
		exp = data->arg1;
		cam = get_property(rw_CAM_READOUT_us);
		shutter = get_property(rw_SHUTTER_DELAY_us);
		start = current_time_us() + data->ts_us + UNIFORM_TIME_DELAY;
	}
};


void start_continuous_acq(const DataPacket* data) {
    AcqParams p(data);

    schedule_shutter_pulse(
		data->N * p.exp + p.shutter,         // duration
		p.start + p.cam - p.shutter,         // timestamp
		1, 0, false);						 // just once
    
	// Single pulse to read out the camera
    schedule_pulse(CAMERA_PIN, default_pulse_duration_us, p.start, 1, 0, false);
	
	// N+1 pulses to trigger camera in sync mode
    schedule_pulse(CAMERA_PIN, default_pulse_duration_us, p.start + p.cam,
				   data->N + 1, p.exp, false);
}


void start_stroboscopic_acq(const DataPacket* data) {
    AcqParams p(data);

    uint32_t frame_period = std::max(p.exp + p.cam + p.shutter, data->interv_us);
    
    schedule_shutter_pulse(p.exp, p.start, data->N, frame_period, false);
    schedule_pulse(CAMERA_PIN, p.exp, p.start + p.shutter,
				   data->N, frame_period, false);
}


void start_ALEX_acq(const DataPacket* data) {
    AcqParams p(data);

    uint32_t N_ch = _count_set_bits(get_property(rw_SELECTED_LASERS));
    uint32_t frame_duration = p.exp + p.cam + p.shutter;
    uint32_t burst_period = std::max(N_ch * frame_duration, data->interv_us);

    uint32_t frame_start = p.start;
    for (uint32_t i = 0; i < 4; ++i) {
	    if (pins[shutter_pins[i]].is_active()) { // laser is enabled
		    schedule_pulse(pins[shutter_pins[i]].pin_idx, p.exp, frame_start, data->N, burst_period, false);
		    schedule_pulse(CAMERA_PIN, p.exp, frame_start + p.shutter, data->N, burst_period, false);
		    frame_start += frame_duration;
	    }
    }
}
