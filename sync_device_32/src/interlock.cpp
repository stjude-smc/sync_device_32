/*
 * interlock.cpp
 *
 * Created: 10/23/2024 5:08:52 PM
 *  Author: rkiselev
 */ 

#include "interlock.h"

volatile bool intlck_match_1 = false;
volatile bool intlck_match_2 = false;

void _init_interlock_timer()
{
    sysclk_enable_peripheral_clock(ID_INTLCK_TC);
    tc_init(INTLCK_TC, INTLCK_TC_CH,
        SYS_TC_CMR_TCCLKS_TIMER_CLOCK |  // same prescaler as the system timer
        TC_CMR_WAVE |                 // waveform generation mode
        TC_CMR_EEVT_XC0 |             // External event selection - enables TIOB
#ifdef INTLCK_TIOA
        // TIOA configuration
        TC_CMR_ASWTRG_SET |           // set A on timer start
        TC_CMR_ACPA_CLEAR |           // clear A on compare event B
        TC_CMR_ACPC_SET |             // set A on compare event C
#else        
        // TIOB configuration
        TC_CMR_BSWTRG_SET |           // set B on timer start
        TC_CMR_BCPB_CLEAR |           // clear B on compare event B
        TC_CMR_BCPC_SET |             // set B on compare event C
#endif        
        TC_CMR_WAVSEL_UP_RC       // restart timer on event C
    );

#ifdef INTLCK_TIOA
    tc_write_ra(INTLCK_TC, INTLCK_TC_CH, us2cts(INTLCK_TC_PERIOD_US >> 4));
	tc_enable_interrupt(INTLCK_TC, INTLCK_TC_CH, TC_IER_CPAS);
#else
    tc_write_rb(INTLCK_TC, INTLCK_TC_CH, us2cts(INTLCK_TC_PERIOD_US >> 4));
	tc_enable_interrupt(INTLCK_TC, INTLCK_TC_CH, TC_IER_CPBS);
#endif
    tc_write_rc(INTLCK_TC, INTLCK_TC_CH, us2cts(INTLCK_TC_PERIOD_US));
	tc_enable_interrupt(INTLCK_TC, INTLCK_TC_CH, TC_IER_CPCS);
	
	NVIC_EnableIRQ(INTLCK_TC_IRQn);
	NVIC_SetPriority(INTLCK_TC_IRQn, 3); // The highest priority is 0 (reserved for watchdog)
}


void init_interlock()
{
    _init_interlock_timer();

    sysclk_enable_peripheral_clock(ioport_pin_to_port_id(INTLCK_OUT));

    ioport_set_pin_mode(INTLCK_OUT, INTLCK_OUT_PERIPH);
    ioport_disable_pin(INTLCK_OUT);

    tc_start(INTLCK_TC, INTLCK_TC_CH);
}


void INTLCK_TC_Handler()
{
    // Read Timer Counter Status to clear the interrupt flag
    uint32_t status = tc_get_status(INTLCK_TC, INTLCK_TC_CH);
    
    // RA or RB match - output went from high to low
	if ((status & TC_SR_CPAS) || (status & TC_SR_CPBS)) {
		intlck_match_1 = ioport_get_pin_level(INTLCK_IN) == 0;
	}

	// RC match - output went from low to high
	if (status & TC_SR_CPCS) {
		intlck_match_2 = ioport_get_pin_level(INTLCK_IN) == 1;
	}
	
	// Check if both conditions match
	if (intlck_match_1 && intlck_match_2)
	{
		pins[CY2_PIN].set_level(0);
	}
	else
	{
		pins[CY2_PIN].set_level(1);
	}
}

