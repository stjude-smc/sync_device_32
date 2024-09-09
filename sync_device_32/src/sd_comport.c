#include "sd_comport.h"
#include <string.h>


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
void sd_send_chr(const char chr)
{
	uart_write(UART, chr);
}


// Send data to host using DMA controller
void sd_send_string(const char *cstring)
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



void poll_UART(void)
{
	union Data data;
	if (sd_rx_string(data.bytes, 5) == OK)
	{
		parse_UART_command(data);
	}	
}

void parse_UART_command(const union Data data)
{
	sd_send_string("Got five bytes before timeout!\n");
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
		if (sd_rx_byte(bytearray[i]) == ERR_TIMEOUT)
			return ERR_TIMEOUT;

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
	
	//NVIC_EnableIRQ(TC0_IRQn + TC_CHAN);
}


void TC0_Handler(void){
	// Read the status register to clear the interrupt flag
	tc_get_status(TC0, TC_CHAN);

	// Toggle LED
	ioport_toggle_pin_level(CY5_PIN);

	tc_start(TC0, TC_CHAN);
}