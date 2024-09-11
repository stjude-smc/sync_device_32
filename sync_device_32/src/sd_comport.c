#include "sd_comport.h"
#include <string.h>
#include "sd_events.h"
#include "sd_triggers.h"

// Memory buffer for DMA transmission
static uint8_t tx_buffer[UART_BUFFER_SIZE];
static uint8_t rx_buffer[UART_BUFFER_SIZE];


// Prototypes of internal functions
void _DMA_tx_wait(Pdc* p_uart_pdc);
void _parse_UART_command(const union Data data);
void _init_UART_TC(void);
void _init_UART_DMA_rx(uint8_t size);


void sd_init_UART(void)
{
	// Enable clock for PIOA
	pmc_enable_periph_clk(ID_PIOA);
	
	// Give control over pins from PIO to UART
	pio_set_peripheral(PIOA, PIO_PERIPH_A, PIO_PA8A_URXD | PIO_PA9A_UTXD);
	
	// Enable pull-up resistor on these pins
	pio_pull_up(PIOA, PIO_PA8A_URXD | PIO_PA9A_UTXD, PIO_PULLUP);
	
	// Enable UART clock
	sysclk_enable_peripheral_clock(ID_UART);
	
	// Initialize UART with our specific settings
	const sam_uart_opt_t uart_settings = {
		.ul_mck = BOARD_MCK,            // Master clock frequency
		.ul_baudrate = UART_BAUDRATE,   // Desired baudrate
		.ul_mode = UART_MR_PAR_NO       // No parity, normal channel mode
	};
		
	// Init UART and enable receiver and transmitter
	uint32_t status = uart_init(UART, &uart_settings);
	
	if (status == 0) {
		ioport_set_pin_level(PIO_PB27_IDX, true);
	} else {
		ioport_set_pin_level(PIO_PB27_IDX, false);
	}
	
	uart_enable_tx(UART);
	uart_enable_rx(UART);
	
	// Enable DMA for UART transmissions
	pdc_enable_transfer(uart_get_pdc_base(UART), PERIPH_PTCR_TXTEN | PERIPH_PTCR_RXTEN);
	_init_UART_DMA_rx(5);
	
	// Enable interrupts
	uart_enable_interrupt(UART, UART_IER_RXRDY | UART_IER_ENDRX);
	NVIC_EnableIRQ(UART_IRQn);
	
	// Initialize TC for timeout detection
	_init_UART_TC();
}



// Send data to host using DMA controller
void sd_tx(const char *cstring)
{
	Pdc* p_uart_pdc = uart_get_pdc_base(UART);
	
	// Wait if previous transmission didn't finish
	_DMA_tx_wait(p_uart_pdc);
	
	// Save provided string to the outgoing buffer because `cstring`
	// will be destroyed once we exit this function
	uint8_t size = strlen(cstring);
	memcpy(tx_buffer, cstring, size);
	
	// Configure PDC for transmission
	pdc_packet_t pdc_uart_tx_packet;
	pdc_uart_tx_packet.ul_addr = (uint32_t)tx_buffer;
	pdc_uart_tx_packet.ul_size = size;
	
	pdc_tx_init(p_uart_pdc, &pdc_uart_tx_packet, NULL);
}


void _init_UART_DMA_rx(uint8_t size)
{
	Pdc* p_uart_pdc = uart_get_pdc_base(UART);

	// Configure PDC for reception
	pdc_packet_t pdc_uart_rx_packet;
	pdc_uart_rx_packet.ul_addr = (uint32_t)rx_buffer;
	pdc_uart_rx_packet.ul_size = size;
	
	pdc_rx_init(p_uart_pdc, &pdc_uart_rx_packet, NULL);

	pdc_enable_transfer(p_uart_pdc, PERIPH_PTCR_TXTEN | PERIPH_PTCR_RXTEN);
}

// Wait until DMA controller finishes data transmission
void _DMA_tx_wait(Pdc* p_uart_pdc)
{
	while (pdc_read_tx_counter(p_uart_pdc) > 0)
	{
		;
	}
}


// Timer/counter for UART timeout
void _init_UART_TC(void)
{
	sysclk_enable_peripheral_clock(ID_UART_TC);
	
	tc_init(UART_TC, UART_TC_CH,
			TC_CMR_TCCLKS_TIMER_CLOCK2 |   // Prescaler MCK/8
			TC_CMR_WAVSEL_UP_RC            // Count up to TC_RC
	);
	
	uint32_t rc_value = (sysclk_get_peripheral_hz() / 8 / 1000) * UART_TIMEOUT;
	tc_write_rc(UART_TC, UART_TC_CH, rc_value);

	// Enable the interrupt on RC compare
	tc_enable_interrupt(UART_TC, UART_TC_CH, TC_IER_CPCS);
	
	NVIC_EnableIRQ(UART_TC_IRQn);
	
	tc_start(UART_TC, UART_TC_CH);
}


void UART_TIMEOUT_Handler(void)
{
	uint32_t status = tc_get_status(UART_TC, UART_TC_CH);
	
	if (status & TC_SR_CPCS) // Communication timeout
	{
		_init_UART_DMA_rx(5); // Reset the receiver buffer
	}
}

void UART_Handler(void)
{
	// A character arrived - reset the timer for communication timeout
	tc_start(UART_TC, UART_TC_CH);

    // Check if the PDC transfer is complete
    if (uart_get_status(UART) & UART_SR_ENDRX) {
		union Data data;
		memcpy(&data, &rx_buffer, 5);
	    _parse_UART_command(data);

	    // Reinitialize PDC for the next transfer
		_init_UART_DMA_rx(5);
    }
}


/************************************************************************/
/* Implementation of the communication protocol                         */
/************************************************************************/

void _parse_UART_command(const union Data data)
{
	switch (data.cmd)
	{
		// send pulse
		case 'P':
			ioport_set_pin_level(PIO_PA16_IDX, 1);
			pulse_table[0].pending = true;
			pulse_table[0].pin = PIO_PA16_IDX;
			pulse_table[0].timestamp = tc_read_cv(OTE_TC, OTE_TC_CH) + us2cts(data.uint32_value);
			pulse_table[0].polarity = 1;
			break;
		// Set laser shutters and ALEX
		case 'L':
		sd_tx("sys.lasers_in_use = data.lasers.lasers_in_use & SHUTTERS_MASK\n");
		sd_tx("sys.ALEX_enabled = data.lasers.ALEX_enabled;\n");
		sd_tx("reset_lasers();\n");
		sd_tx("UART_tx_ok();\n");
		sd_tx("if (sys.status == MANUAL)\n");
		sd_tx("{\n");
			sd_tx("set_lasers(sys.lasers_in_use);\n");
		sd_tx("}\n");
		break;

		// Set acquisition period between frames/bursts
		case 'A':
		sd_tx("sys.acq_period_us = data.uint32_value\n");
		sd_tx("UART_tx_ok();\n");
		break;

		// Set exposure time
		case 'E':
		sd_tx("sys.exp_time_us = data.uint32_value\n");
		sd_tx("UART_tx_ok();\n");
		break;

		// Set shutter delay
		case 'D':
		sd_tx("sys.shutter_delay_us = data.uint32_value\n");
		sd_tx("UART_tx_ok();\n");
		break;

		// Set camera readout delay
		case 'I':
		sd_tx("sys.cam_readout_us = data.uint32_value\n");
		sd_tx("UART_tx_ok();\n");
		break;

		// Set fluidics trigger
		case 'F':
		sd_tx("sys.fluidics_frame = data.int32_value\n");
		sd_tx("UART_tx_ok();\n");
		break;

		// Start stroboscopic acquisition
		case 'S':
		sd_tx("UART_tx_ok()\n");
		sd_tx("start_stroboscopic_acq(data.uint32_value);\n");
		break;

		// Start continuous acquisition
		case 'C':
			tc_write_rc(TC1, 0, us2cts(data.uint32_value));
			tc_enable_interrupt(TC1, 0, TC_IER_CPCS);
			tc_start(TC1, 0);
		break;

		// Manually open laser shutters
		case 'M':
			set_lasers(data.lasers.lasers_in_use);
		sd_tx("if (sys.status != IDLE)\n");
		sd_tx("{\n");
			sd_tx("	UART_tx_err(\"M: Not in the IDLE state\");\n");
			sd_tx("	break;\n");
		sd_tx("}\n");
		sd_tx("if (sys.lasers_in_use == 0)\n");
		sd_tx("{\n");
			sd_tx("	UART_tx_err(\"M: All lasers are disabled\");\n");
			sd_tx("	break;\n");
		sd_tx("}\n");
		sd_tx("UART_tx_ok();\n");
		sd_tx("set_lasers(sys.lasers_in_use);\n");
		sd_tx("sys.status = MANUAL;\n");
		break;

		// Stop acquisition
		case 'Q':
		sd_tx("sys.n_frames = data.uint32_value\n");
		sd_tx("UART_tx_ok();\n");
		sd_tx("stop_acq();\n");
		break;

		default:
		break;
	sd_tx("\n");

	}
}