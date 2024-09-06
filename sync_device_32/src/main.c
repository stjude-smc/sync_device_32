/**
 * \file
 *
 * \brief Synchronization device for laser shutters and cameras
 *
 */

#include <asf.h>
#include "sd_globals.h"
#include "sd_comport.h"

// Declaration of function prototypes
void init_IO(void);


// ENTRY POINT
int main(void)
{
	// Initialize the board
	sysclk_init();   // Initialize the system clock
	board_init();    // Initialize the board (configures default pins)
	
	
	// Enable peripheral clock for PIOA (controls PA8 and PA9) - for UART
	pmc_enable_periph_clk(ID_PIOA);

	// Disable PIO control for PA8 and PA9, and assign to Peripheral A (UART0)
	pio_set_peripheral(PIOA, PIO_PERIPH_A, PIO_PA8A_URXD | PIO_PA9A_UTXD);

	// Optional: Enable the pull-up on PA8 and PA9 if not needed
	pio_pull_up(PIOA, PIO_PA8A_URXD | PIO_PA9A_UTXD, 1);

    init_IO();
	
	const uart_settings_t settings = {
		.baud_rate = 115200,
		.parity = UART_PARITY_NO,
		.ch_mode = UART_CHMODE_NORMAL
	};
    
	sysclk_enable_peripheral_clock(BOARD_ID_USART);
	my_uart_init(&settings);

    // Notify the host that we are ready
    //UART_tx("Sync device is ready. Firmware version: ");
    //UART_tx(VERSION);


	char i = 32;
	while (1)
	{
//		buf = uart_read_char();
		uart_write_char(i);
		if (i++ > 127){
			i = 32;
		}


		// Set the pin high
		ioport_set_pin_level(CY2_PIN, true);
		ioport_set_pin_level(CY3_PIN, false);
		delay_ms(50);  // Wait for 1 second
		
		// Set the pin low
		ioport_set_pin_level(CY2_PIN, false);
		ioport_set_pin_level(CY3_PIN, true);
		ioport_toggle_pin_level(CY5_PIN);
		delay_ms(50);  // Wait for 1 second*/
		
		;
		//poll_UART();
	}
}


void init_IO(void)
{
	// Initialize the IOPORT system
	ioport_init();   // Initialize the IOPORT system

	// Configure the laser shutters as outputs
	ioport_set_port_dir(SHUTTERS_PORT, SHUTTERS_MASK, IOPORT_DIR_OUTPUT);

	// Configure the fluidic trigger as output
	ioport_set_pin_dir(FLUIDIC_PIN, IOPORT_DIR_OUTPUT);

	// Configure the camera trigger as output
	ioport_set_pin_dir(CAMERA_PIN, IOPORT_DIR_OUTPUT);
}
