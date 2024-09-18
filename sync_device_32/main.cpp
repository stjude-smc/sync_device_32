/*
 * GccApplication2.cpp
 *
 * Created: 9/17/2024 3:00:55 PM
 * Author : rkiselev
 */ 


extern "C" {
	#include "asf.h"
}


#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#include <queue>
#include <vector>
#include <functional>

#include <new>
#include <cstdio>

void my_new_handler() {
	// Handle out-of-memory error here
	printf("Out of memory!\n");
	// You can abort or take other actions
	std::abort();
}


extern "C" {
// Implementation of _write function for printf
int _write(int file, char *ptr, int len) {
	for (int i = 0; i < len; i++) {
		// Wait until UART is ready to transmit
		while (!uart_is_tx_ready(UART)) {}
		uart_write(UART, ptr[i]);
	}
	return len;
}

int _read(int file, char *ptr, int len) {
	// This is a minimal implementation; it doesn't actually read any data.
	// It needs to return the number of bytes read (0 in this case).
	return 0;
}

}


typedef struct Event
{
	void		  (*func)(uint32_t arg1, uint32_t arg2);
	uint32_t	  arg1;
	uint32_t	  arg2;
	uint32_t	  timestamp;
	uint32_t	  N;
	uint32_t	  interval;
	bool		  active;

	bool operator<(const Event& other) const {
		return this->timestamp > other.timestamp;
	}
} Event;  // 28 bytes



void setup_uart() {
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
		.ul_baudrate = 115200,   // Desired baudrate
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


	// Set the custom stream buffer for std::cout
//	std::cout.rdbuf(&uartStream);
}

void send_pulse(void){
	ioport_set_pin_level(PIO_PA16_IDX, 1);
	ioport_set_pin_level(PIO_PA16_IDX, 0);
}



int main() {
	std::set_new_handler(my_new_handler);
	// Initialize ASF, board, and UART
	sysclk_init();
	board_init();
	setup_uart();

	pmc_enable_periph_clk(ID_TRNG);
	trng_enable(TRNG);

	printf("Hello, SAM3X!\n");

    // Create a table of events
    std::priority_queue<Event> EventTable;
	

	ioport_set_pin_dir(PIO_PA16_IDX, IOPORT_DIR_OUTPUT);
	delay_ms(5);
	

	while (1) {

		uint32_t N = 0;
		while(!uart_is_rx_ready(UART)){}

		uint8_t data;
		uart_read(UART, &data);
		N = ((uint32_t) data) << 3;

		
		printf("N = %lu\n", N);

		Event e;
		for (uint32_t i = 0; i < N; i++){
			e.interval = i;
			e.timestamp = trng_read_output_data(TRNG) >> 20;
			EventTable.push(e);
			send_pulse();
		}
		ioport_set_pin_level(PIO_PA16_IDX, IOPORT_PIN_LEVEL_LOW);

		int i = 0;
		while (!EventTable.empty()) {
			Event retrieved_event = EventTable.top();
			printf(" Element %d = %lu\n", i++, retrieved_event.timestamp);     // Access the top (largest) element
			EventTable.pop();                     // Remove the top element
		}




	}
}

