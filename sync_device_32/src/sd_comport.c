#include "sd_comport.h"
#include <string.h>
#include "sd_events.h"

// Memory buffer for DMA transmission
static uint8_t tx_buffer[UART_BUFFER_SIZE];

#define TC_CHAN 0

void _DMA_tx_wait(Pdc* p_uart_pdc);
void parse_UART_command(const union Data data);
void _init_UART_TC(void);

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
	pdc_enable_transfer(uart_get_pdc_base(UART), PERIPH_PTCR_TXTEN);
	
	// Initialize TC0 for timeout detection
	_init_UART_TC();
}


// Send one character to host
void sd_tx_chr(const char chr)
{
	uart_write(UART, chr);
}


// Send data to host using DMA controller
void sd_tx_string(const char *cstring)
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


// Wait until DMA controller finishes data transmission
void _DMA_tx_wait(Pdc* p_uart_pdc)
{
	while (pdc_read_tx_counter(p_uart_pdc) > 0)
	{
		;
	}
}



void sd_poll_UART(void)
{
	union Data data;
	if (sd_rx_string(data.bytes, 5) == OK)
	{
		parse_UART_command(data);
	}	
}


errcode sd_rx_byte(uint8_t *byte)
{
	tc_start(TC0, TC_CHAN);
	
	while (!uart_is_rx_ready(UART)) // Wait for data...
	{
		if (tc_get_status(TC0, TC_CHAN) & TC_IER_CPCS)  // until a timeout is detected
			return ERR_TIMEOUT;
	}  
	
	uart_read(UART, byte);
	tc_start(TC0, TC_CHAN);
	return OK;
}

errcode sd_rx_string(uint8_t *bytearray, uint8_t size)
{
	for (uint8_t i = 0; i < size; i++)
		if (sd_rx_byte(&bytearray[i]) == ERR_TIMEOUT)
		{
			ioport_set_pin_level(CY5_PIN, 1);  // FIXME - delete
			return ERR_TIMEOUT;
		}

	ioport_set_pin_level(CY5_PIN, 0);  // FIXME - delete
	return OK;
}



// Timer/counter 0 channel 0 is used for UART timeout
void _init_UART_TC(void)
{
	sysclk_enable_peripheral_clock(ID_TC0);
	
	tc_init(TC0, TC_CHAN,
			TC_CMR_TCCLKS_TIMER_CLOCK2 |   // Prescaler MCK/8
			TC_CMR_WAVSEL_UP_RC            // Count up to TC_RC
	);
	
	uint32_t rc_value = (sysclk_get_peripheral_hz() / 8 / 1000) * UART_TIMEOUT;
	tc_write_rc(TC0, TC_CHAN, rc_value);

	// Enable the interrupt on RC compare
	tc_enable_interrupt(TC0, TC_CHAN, TC_IER_CPCS);
	
	tc_start(TC0, TC_CHAN);
}



/************************************************************************/
/* Implementation of the communication protocol                         */
/************************************************************************/

void parse_UART_command(const union Data data)
{
	switch (data.cmd)
	{
		// Set laser shutters and ALEX
		case 'L':
		sd_tx_string("sys.lasers_in_use = data.lasers.lasers_in_use & SHUTTERS_MASK\n");
		sd_tx_string("sys.ALEX_enabled = data.lasers.ALEX_enabled;\n");
		sd_tx_string("reset_lasers();\n");
		sd_tx_string("UART_tx_ok();\n");
		sd_tx_string("if (sys.status == MANUAL)\n");
		sd_tx_string("{\n");
			sd_tx_string("set_lasers(sys.lasers_in_use);\n");
		sd_tx_string("}\n");
		break;

		// Set acquisition period between frames/bursts
		case 'A':
		sd_tx_string("sys.acq_period_us = data.uint32_value\n");
		sd_tx_string("UART_tx_ok();\n");
		break;

		// Set exposure time
		case 'E':
		sd_tx_string("sys.exp_time_us = data.uint32_value\n");
		sd_tx_string("UART_tx_ok();\n");
		break;

		// Set shutter delay
		case 'D':
		sd_tx_string("sys.shutter_delay_us = data.uint32_value\n");
		sd_tx_string("UART_tx_ok();\n");
		break;

		// Set camera readout delay
		case 'I':
		sd_tx_string("sys.cam_readout_us = data.uint32_value\n");
		sd_tx_string("UART_tx_ok();\n");
		break;

		// Set fluidics trigger
		case 'F':
		sd_tx_string("sys.fluidics_frame = data.int32_value\n");
		sd_tx_string("UART_tx_ok();\n");
		break;

		// Start stroboscopic acquisition
		case 'S':
		sd_tx_string("UART_tx_ok()\n");
		sd_tx_string("start_stroboscopic_acq(data.uint32_value);\n");
		break;

		// Start continuous acquisition
		case 'C':
			tc_write_rc(TC1, 0, tc_read_rc(TC1, 0) >> 1);
		
			//tc_write_rc(TC1, 0, data.uint32_value);
			//TC1->TC_CHANNEL[0].TC_RC = data.uint32_value;
			//tc_enable_interrupt(TC1, 0, TC_IER_CPCS);

			tc_start(TC1, 0);
		break;

		// Manually open laser shutters
		case 'M':
		sd_tx_string("if (sys.status != IDLE)\n");
		sd_tx_string("{\n");
			sd_tx_string("	UART_tx_err(\"M: Not in the IDLE state\");\n");
			sd_tx_string("	break;\n");
		sd_tx_string("}\n");
		sd_tx_string("if (sys.lasers_in_use == 0)\n");
		sd_tx_string("{\n");
			sd_tx_string("	UART_tx_err(\"M: All lasers are disabled\");\n");
			sd_tx_string("	break;\n");
		sd_tx_string("}\n");
		sd_tx_string("UART_tx_ok();\n");
		sd_tx_string("set_lasers(sys.lasers_in_use);\n");
		sd_tx_string("sys.status = MANUAL;\n");
		break;

		// Stop acquisition
		case 'Q':
		sd_tx_string("sys.n_frames = data.uint32_value\n");
		sd_tx_string("UART_tx_ok();\n");
		sd_tx_string("stop_acq();\n");
		break;

		default:
		break;
	sd_tx_string("\n");

	}
}