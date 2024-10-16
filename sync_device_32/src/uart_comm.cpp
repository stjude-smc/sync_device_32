#include "uart_comm.h"
#include "events.h"

#define RSTC_KEY  0xA5000000  // password for reset controller

/************************************************************************/
/*                  UART RECEIVE/TRANSMIT BUFFERS                       */
/************************************************************************/

// Outgoing UART message class
class UartTxMessage {
private:
	char *p_buf;
	pdc_packet_t uart_tx_packet;
	
public:
	bool is_transmitted;

	UartTxMessage (const char* data, uint32_t len)
	{
		this->p_buf = new char[len];
		this->uart_tx_packet.ul_addr = (uint32_t) this->p_buf;
		this->uart_tx_packet.ul_size = len;
		memcpy(p_buf, data, len);
		this->is_transmitted = false;
	}
	
	~UartTxMessage()
	{
		delete[] this->p_buf;
	}
	
	void transmit()
	{
		pdc_tx_init(uart_get_pdc_base(UART), &uart_tx_packet, nullptr);
		this->is_transmitted = true;
	}
};

// Memory buffer for outgoing DMA transmissions
std::queue<UartTxMessage*> tx_queue;

//////////////////////////////////////////////////////////////////////////

// Memory buffers for incoming DMA transmissions
static volatile uint8_t rx_buffer_A[UART_BUFFER_SIZE];
static volatile uint8_t rx_buffer_B[UART_BUFFER_SIZE];
volatile bool rx_buffer_ready = false;

// Pointer to buffer with pending data to be processed
volatile uint8_t *rx_filled_buffer_p = NULL;


/************************************************************************/
/*                  INTERNAL FUNCTION PROTOTYPES                        */
/************************************************************************/
void _DMA_tx_wait(Pdc* p_uart_pdc);
void _parse_UART_command(const DataPacket *data);
void _init_UART_TC(void);
inline void _init_UART_DMA_rx(size_t size);
void _send_event_queue();

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
	uart_init(UART, &uart_settings);
	
	uart_enable_tx(UART);
	uart_enable_rx(UART);
	
	// Enable DMA for UART transmissions
	_init_UART_DMA_rx(sizeof(DataPacket));
	
	// Enable interrupts
	uart_enable_interrupt(UART, UART_IER_RXRDY | UART_IER_ENDRX);
	NVIC_EnableIRQ(UART_IRQn);
	NVIC_SetPriority(UART_IRQn, 2);  // the highest priority is 0
	
	// Initialize TC for timeout detection
	_init_UART_TC();
}



// Send data to host using DMA controller
void uart_tx(const char *cstring)
{
	uart_tx(cstring, strlen(cstring));
}

void uart_tx(const char *data, uint32_t len)
{
	UartTxMessage * p_uart_msg = new UartTxMessage(data, len);
	
	tx_queue.push(p_uart_msg);
	uart_get_status(UART); // clear pending interrupt flags
	uart_enable_interrupt(UART, UART_IER_ENDTX);
}


void poll_uart()
{
	if (rx_buffer_ready)
	{
		// Parse the content of rx_filled_buffer_p, which contains a DataPacket
		_parse_UART_command((DataPacket *) rx_filled_buffer_p);
		rx_buffer_ready = false;
	}
}


inline void _init_UART_DMA_rx(size_t size)
{
	// Configure PDC for double-buffered reception
	pdc_packet_t pdc_buf;
	pdc_buf.ul_size = size;

	// Swap buffers
	if (rx_filled_buffer_p == rx_buffer_A){
		rx_filled_buffer_p = rx_buffer_B;
		pdc_buf.ul_addr = (uint32_t) rx_buffer_A;
	}else{
		rx_filled_buffer_p = rx_buffer_A;
		pdc_buf.ul_addr = (uint32_t) rx_buffer_B;
	}

	Pdc *pdc_uart_p = uart_get_pdc_base(UART);
	pdc_rx_init(pdc_uart_p, &pdc_buf, NULL);
	pdc_enable_transfer(pdc_uart_p, PERIPH_PTCR_RXTEN | PERIPH_PTCR_TXTEN);
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
	NVIC_SetPriority(UART_TC_IRQn, 15); // the lowest priority is 15
	
	tc_start(UART_TC, UART_TC_CH);
}




/************************************************************************/
/* Implementation of the communication protocol                         */
/************************************************************************/
// This takes about 280-360 us
void _parse_UART_command(const DataPacket *data)
{
	if (strncasecmp(data->cmd, "VER", 3) == 0)
	{
		printf("SYNC DEVICE v%s\n", VERSION);
		printf("System timer prescaler=%lu (1ct=%luns)\n", SYS_TC_PRESCALER, (uint32_t) cts2us(1000));
		printf("Watchdog interval: %lu us \n", wdt_get_us_timeout_period(WDT, BOARD_FREQ_SLCK_XTAL));

	}
	else if (strncasecmp(data->cmd, "GO!", 3) == 0)
	{
		start_sys_timer();
	}
	else if (strncasecmp(data->cmd, "RST", 3) == 0)
	{
		RSTC->RSTC_CR = RSTC_KEY | RSTC_CR_PROCRST;  // processor reset
	}
	else if (strncasecmp(data->cmd, "STP", 3) == 0)  // stop system timer and delete event queue
	{
		stop_burst_func(0, 0);
		stop_sys_timer();
		std::priority_queue<Event>().swap(event_queue);
		
		init_pins();
	}
	else if (strncasecmp(data->cmd, "PIN", 3) == 0)
	{
		schedule_pin(data);
	}
	else if (strncasecmp(data->cmd, "PPL", 3) == 0)
	{
		schedule_pulse(data, true);
	}
	else if (strncasecmp(data->cmd, "NPL", 3) == 0)
	{
		schedule_pulse(data, false);
	}
	else if (strncasecmp(data->cmd, "BST", 3) == 0)
	{
		schedule_burst(data);
	}
	else if (strncasecmp(data->cmd, "TGL", 3) == 0)
	{
		schedule_toggle(data);
	}
	else if (strncasecmp(data->cmd, "ENP", 3) == 0)
	{
		schedule_enable_pin(data);
	}
	else if (strncasecmp(data->cmd, "DSP", 3) == 0)
	{
		schedule_disable_pin(data);
	}
	else if (strncasecmp(data->cmd, "STA", 3) == 0)
	{
		printf("-- SYSTEM STATUS --\n");
		printf("Event queue size: %lu\n", (uint32_t) event_queue.size());
		printf("System counter is %s\n", sys_timer_running ? "RUNNING" : "STOPPED");
		printf("Counter value:  %lu cts, OVF = %lu\n", SYS_TC->TC_CHANNEL[SYS_TC_CH].TC_CV, (uint32_t) (sys_tc_ovf_count >> 32));
		printf("System time: %f s\n", current_time_s());
	}
	else if (strncasecmp(data->cmd, "QUE", 3) == 0)
	{
		_send_event_queue();
	}
	else if (strncasecmp(data->cmd, "FUN", 3) == 0)
	{
		printf("%lu TGL_PIN\n", (uint32_t) &tgl_pin_event_func);
		printf("%lu SET_PIN\n", (uint32_t) &set_pin_event_func);
		printf("%lu BST__ON\n", (uint32_t) &start_burst_func);
		printf("%lu BST_OFF\n", (uint32_t) &stop_burst_func);
	}
	else if (strncasecmp(data->cmd, "INT", 3) == 0)
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
		printf("ERR: unknown command %.3s\n", data->cmd);
	}
}

void _send_event_queue()
{
	Event event;
	std::priority_queue<Event> event_queue_copy = event_queue;

	while (!event_queue_copy.empty())
	{
		event = event_queue_copy.top();

		event_queue_copy.pop();
		uart_tx((char *) &event, sizeof(Event));
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
	
	// Check if the PDC reception is complete - happens ~120us after end of transmission
	if (status & UART_SR_ENDRX)
	{
		// tell the main event loop to process rx_filled_buffer_p
		rx_buffer_ready = true;
		
		// Swap DMA buffers and get ready for the next reception
		_init_UART_DMA_rx(sizeof(DataPacket));
	}
	
	// Check if the PDC transmission is complete
	if (status & UART_SR_ENDTX)
	{
		if (!tx_queue.empty()) // there is pending data transfer
		{
			UartTxMessage* p_uart_msg = tx_queue.front();
			if (p_uart_msg->is_transmitted)
			{
				delete p_uart_msg; // free memory
				tx_queue.pop();    // remove message from the queue
			}
			else
			{
				p_uart_msg->transmit();
			}
		}
		else
		{
			uart_disable_interrupt(UART, UART_IDR_ENDTX);
		}
	}
}