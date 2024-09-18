#include "uart_comm.h"
#include <string.h>
#include <strings.h>

// Memory buffer for DMA transmission
static uint8_t tx_buffer[UART_BUFFER_SIZE];
static uint8_t tx_next_buffer[UART_BUFFER_SIZE];
static uint8_t rx_buffer[UART_BUFFER_SIZE];
//static uint8_t rx_next_buffer[UART_BUFFER_SIZE];


// Prototypes of internal functions
void _DMA_tx_wait(Pdc* p_uart_pdc);
void _parse_UART_command(const DataPacket data);
void _init_UART_TC(void);
void _init_UART_DMA_rx(uint8_t size);


void uart_comm_init(void)
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
	_init_UART_DMA_rx(sizeof(DataPacket));
	
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
	
	pdc_packet_t pdc_uart_tx_packet;
	size_t req_size = strlen(cstring);

	if (pdc_read_tx_counter(p_uart_pdc) > 0)  // pending transmission, use next buffer
	{
		// If not enough space, wait until the next buffer clears
		while (UART_BUFFER_SIZE - pdc_read_tx_next_counter(p_uart_pdc) < req_size)
		{
			;
		}
		// Append more data to the next outgoing buffer
		memcpy(tx_next_buffer + pdc_read_tx_next_counter(p_uart_pdc), cstring, req_size);
		pdc_uart_tx_packet.ul_addr = (uint32_t)tx_next_buffer;
		pdc_uart_tx_packet.ul_size = pdc_read_tx_next_counter(p_uart_pdc) + req_size;
		pdc_tx_init(p_uart_pdc, NULL, &pdc_uart_tx_packet);
		return;
	}
	
	// Copy provided string to the outgoing buffer
	memcpy(tx_buffer, cstring, req_size);
	pdc_uart_tx_packet.ul_addr = (uint32_t)tx_buffer;
	pdc_uart_tx_packet.ul_size = req_size;	
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





/************************************************************************/
/* Implementation of the communication protocol                         */
/************************************************************************/
void _parse_UART_command(const DataPacket data)
{
	if (strncasecmp((char*) data.cmd, "PIN", 3) == 0)
	{
		sd_tx("PIN command ");
	}

	else if (strncasecmp((char*) data.cmd, "CAM", 3) == 0)
	{
		sd_tx("CAM command ");
	}
	else
	{
		sd_tx("unknown command ");
		sd_tx((const char*) data.cmd);
	}
}


/************************************************************************/
/*                   UART-RELATED INTERRUPTS                            */
/************************************************************************/
void UART_TIMEOUT_Handler(void)
{
	uint32_t status = tc_get_status(UART_TC, UART_TC_CH);
	
	if (status & TC_SR_CPCS) // Communication timeout
	{
		_init_UART_DMA_rx(sizeof(DataPacket)); // Reset the receiver buffer
	}
}


void UART_Handler(void)
{
	uint32_t status = uart_get_status(UART);
	// A character arrived - reset the timer for communication timeout
	tc_start(UART_TC, UART_TC_CH);

	// Check if the PDC transfer is complete
	if (status & UART_SR_ENDRX) {
		DataPacket data;
		memcpy(&data, &rx_buffer, sizeof(DataPacket));
		_parse_UART_command(data);

		// Reinitialize PDC for the next transfer
		_init_UART_DMA_rx(sizeof(DataPacket));
	}
}