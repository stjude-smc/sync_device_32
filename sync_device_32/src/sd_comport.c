#include "sd_comport.h"
#include <string.h>


void _DMA_tx_wait(Pdc* p_uart_pdc);


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
	
	pdc_enable_transfer(uart_get_pdc_base(UART), PERIPH_PTCR_TXTEN | PERIPH_PTCR_RXTEN);
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

