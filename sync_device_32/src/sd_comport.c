#include "sd_comport.h"
#include <string.h>
#include "sd_events.h"
#include "sd_triggers.h"
#include "sd_pin_map.h"

// Memory buffer for DMA transmission
static uint8_t tx_buffer[UART_BUFFER_SIZE];
static uint8_t rx_buffer[UART_BUFFER_SIZE];


// Prototypes of internal functions
void _DMA_tx_wait(Pdc* p_uart_pdc);
void _parse_UART_command(const Data data);
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
	_init_UART_DMA_rx(sizeof(Data));
	
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
	
	NVIC_EnableIRQ(UART_TC_IQRn);
	
	tc_start(UART_TC, UART_TC_CH);
}


void UART_TIMEOUT_Handler(void)
{
	uint32_t status = tc_get_status(UART_TC, UART_TC_CH);
	
	if (status & TC_SR_CPCS) // Communication timeout
	{
		_init_UART_DMA_rx(sizeof(Data)); // Reset the receiver buffer
	}
}

void UART_Handler(void)
{
	uint32_t status = uart_get_status(UART);
	// A character arrived - reset the timer for communication timeout
	tc_start(UART_TC, UART_TC_CH);

    // Check if the PDC transfer is complete
    if (status & UART_SR_ENDRX) {
		Data data;
		memcpy(&data, &rx_buffer, sizeof(Data));
	    _parse_UART_command(data);

	    // Reinitialize PDC for the next transfer
		_init_UART_DMA_rx(sizeof(Data));
    }
}


/************************************************************************/
/* Implementation of the communication protocol                         */
/************************************************************************/

void _parse_UART_command(const Data data)
{
	if (strncasecmp((char*) data.cmd, "PIN", 3) == 0)
	{
		sd_tx("PIN command, pin_idx=");
		uint32_t pin_idx = pin_name_to_ioport_id(data.pin);
		ioport_set_pin_dir(pin_idx, IOPORT_DIR_OUTPUT);
		ioport_set_pin_level(pin_idx, data.arg2);
		
		char txt[10];
		itoa(pin_name_to_ioport_id(data.pin), txt, 10);
		sd_tx(txt);
		
		ioport_toggle_pin_level(CAMERA_PIN);
	}
	else if (strncasecmp((char*) data.cmd, "PUL", 3) == 0)
	{
		sd_tx("PUL command ");
	}
	else if (strncasecmp((char*) data.cmd, "CAM", 3) == 0)
	{
		sd_tx("CAM command ");
	}
	else
	{
		sd_tx("unknown command ");
		sd_tx(data.cmd);
	}
	sd_tx("\n");
}