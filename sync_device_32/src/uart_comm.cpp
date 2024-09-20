#include <string.h>
#include <strings.h>

#include "uart_comm.h"
#include "events.h"


// Memory buffer for DMA transmission
static uint8_t tx_buffer[UART_BUFFER_SIZE];
static uint8_t tx_next_buffer[UART_BUFFER_SIZE];
static uint8_t rx_buffer[UART_BUFFER_SIZE];
//static uint8_t rx_next_buffer[UART_BUFFER_SIZE];


/************************************************************************/
/*                  INTERNAL FUNCTION PROTOTYPES                        */
/************************************************************************/
void _DMA_tx_wait(Pdc* p_uart_pdc);
void _parse_UART_command(const DataPacket data);
void _init_UART_TC(void);
void _init_UART_DMA_rx(uint8_t size);
void _print_event_status(Event event);

void init_uart_comm(void)
{
	// Enable clock for PIOA
	sysclk_enable_peripheral_clock(ID_PIOA);
	
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
	NVIC_SetPriority(UART_IRQn, 1);
	
	// Initialize TC for timeout detection
	_init_UART_TC();
}



// Send data to host using DMA controller
void sd_tx(const char *cstring)
{
	sd_tx(cstring, strlen(cstring));
}

void sd_tx(const char *buf, uint32_t len)
{
	Pdc* p_uart_pdc = uart_get_pdc_base(UART);
	
	pdc_packet_t pdc_uart_tx_packet;

	if (pdc_read_tx_counter(p_uart_pdc) > 0)  // pending transmission, use next buffer
	{
		// If not enough space, wait until the next buffer clears
		while (UART_BUFFER_SIZE - pdc_read_tx_next_counter(p_uart_pdc) < len)
		{
			;
		}
		// Append more data to the next outgoing buffer
		memcpy(tx_next_buffer + pdc_read_tx_next_counter(p_uart_pdc), buf, len);
		pdc_uart_tx_packet.ul_addr = (uint32_t)tx_next_buffer;
		pdc_uart_tx_packet.ul_size = pdc_read_tx_next_counter(p_uart_pdc) + len;
		pdc_tx_init(p_uart_pdc, NULL, &pdc_uart_tx_packet);
		return;
	}
	
	// Copy provided string to the outgoing buffer
	memcpy(tx_buffer, buf, len);
	pdc_uart_tx_packet.ul_addr = (uint32_t)tx_buffer;
	pdc_uart_tx_packet.ul_size = len;
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
	NVIC_SetPriority(UART_TC_IRQn, 0); // low priority
	
	tc_start(UART_TC, UART_TC_CH);
}




/************************************************************************/
/* Implementation of the communication protocol                         */
/************************************************************************/
void _parse_UART_command(const DataPacket data)
{
	if (strncasecmp(data.cmd, "PIN", 3) == 0)
	{
		printf("PIN command, arg1 = %lu, arg2 = %lu, ts=%lu\n", data.arg1, data.arg2, data.timestamp);
		schedule_pin(data);
	}
	else if (strncasecmp(data.cmd, "PUL", 3) == 0)
	{
		printf("PUL command\n");
		schedule_pulse(data);
	}
	else if (strncasecmp(data.cmd, "TGL", 3) == 0)
	{
		printf("TGL command\n");
		schedule_toggle(data);
	}
	else if (strncasecmp(data.cmd, "STA", 3) == 0)
	{
		printf("-- SYSTEM STATUS --\n");
		printf("Event queue size: %lu\n", (uint32_t) event_queue.size());
		printf("Current system time:  %lu cts\n", current_time_cts());
		if (!event_queue.empty())
		{
			_print_event_status(event_queue.top());

		}
	}
	else if (strncasecmp(data.cmd, "INT", 3) == 0)
	{
		// Check the default priority of Timer/Counter 0 interrupt
		uint32_t sys_tc_priority = NVIC_GetPriority(SYS_TC_IRQn);

		// Check the default priority of Timer/Counter 0 interrupt
		uint32_t uart_tc_priority = NVIC_GetPriority(UART_TC_IRQn);

		// Check the default priority of UART interrupt
		uint32_t uart_priority = NVIC_GetPriority(UART_IRQn);
		
		printf("-- SYSTEM INTERRUPT PRIORITIES --\n");
		printf("System timer: %lu\n", sys_tc_priority);
		printf("UART:         %lu\n", uart_priority);
		printf("UART timer:   %lu\n", uart_tc_priority);
	}
	else
	{
		printf("unknown command %.3s\n", data.cmd);
	}
}

void _print_event_status(Event event)
{
	printf("Next event timestamp: %lu cts\n", event.timestamp);
	printf("Next event arg1: %lu\n", event.arg1);
	printf("Next event arg2: %lu\n", event.arg2);
	printf("Next event N:    %lu\n", event.N);
	printf("Next event intev:%lu\n", event.interval);
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