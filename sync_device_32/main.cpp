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

void tp(void){
	ioport_toggle_pin_level(PIO_PA16_IDX);
}

int main() {
	// Initialize ASF, board, and UART
	sysclk_init();
	board_init();
	setup_uart();


	printf("Hello, SAM3X!\n");

	int address = 0xDEADBEEF;
	printf("The address is 0x%X\n", address);

	float pi = 3.14159;
	printf("The value of pi is '%.2f'\n", pi);

	const char* message = "UART communication established!";
	printf("%s\n", message);


	int number = 42;
	printf("The number is %d\n", number);

    int count = 10;
    float average = 5.75;
    printf("Count: %d, Average: '%.2f'\n", count, average);


    // Create a priority queue (max heap by default)
    std::priority_queue<int> maxHeap;

    // Insert elements into the max heap
	
	ioport_set_pin_dir(PIO_PA16_IDX, IOPORT_DIR_OUTPUT);
	delay_ms(5);
	
    maxHeap.push(10); tp();
    maxHeap.push(30); tp();
    maxHeap.push(20); tp();
    maxHeap.push(5); tp();
    maxHeap.push(92); tp();
    maxHeap.push(11); tp();
    maxHeap.push(17); tp();
    maxHeap.push(53); tp();
    maxHeap.push(6); tp();
    maxHeap.push(2); tp();
    maxHeap.push(13); tp();

	int i = 0;
    while (!maxHeap.empty()) {
	    printf("Element %d = %d\n", i++, maxHeap.top());     // Access the top (largest) element
	    maxHeap.pop();                     // Remove the top element
    }

	while (1) {
		// Your main loop
	}
}

