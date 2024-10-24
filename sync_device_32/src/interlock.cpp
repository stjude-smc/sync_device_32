/*
 * interlock.cpp
 *
 * Created: 10/23/2024 5:08:52 PM
 *  Author: rkiselev
 */ 

#include "interlock.h"

void _init_interlock_timer()
{
    sysclk_enable_peripheral_clock(ID_INTLCK_TC);
    tc_init(INTLCK_TC, INTLCK_TC_CH,
        TC_CMR_TCCLKS_TIMER_CLOCK4 |  // 1/128 pre-scaler (~1.56us per count)
        TC_CMR_WAVE |                 // waveform generation mode
        TC_CMR_EEVT_XC0 |             // External event selection - enables TIOB
/*
        // TIOA configuration
        TC_CMR_ASWTRG_SET |           // set A on timer start
        TC_CMR_ACPA_CLEAR |           // clear A on compare event B
        TC_CMR_ACPC_SET |             // set A on compare event C
*/        
        // TIOB configuration
        TC_CMR_BSWTRG_SET |           // set B on timer start
        TC_CMR_BCPB_CLEAR |           // clear B on compare event B
        TC_CMR_BCPC_SET |             // set B on compare event C
        
        TC_CMR_WAVSEL_UPDOWN_RC       // restart timer on event C
    );

    tc_write_ra(INTLCK_TC, INTLCK_TC_CH, 250);
    tc_write_rb(INTLCK_TC, INTLCK_TC_CH, 250);
    tc_write_rc(INTLCK_TC, INTLCK_TC_CH, 1000);
}

void init_interlock()
{
    _init_interlock_timer();

    sysclk_enable_peripheral_clock(ioport_pin_to_port_id(INTLCK_OUT));

    ioport_set_pin_mode(INTLCK_OUT, INTLCK_OUT_PERIPH);
    ioport_disable_pin(INTLCK_OUT);

    tc_start(INTLCK_TC, INTLCK_TC_CH);
}

bool check_interlock()
{

}
