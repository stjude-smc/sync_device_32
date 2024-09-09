/*
 * sd_comport.h
 *
 * Created: 9/5/2024 4:31:45 PM
 *  Author: rkiselev
 */ 

#pragma once
#include <asf.h>
#include <sd_globals.h>

// Memory buffers for transmission and reception
static uint8_t tx_buffer[UART_BUFFER_SIZE];


void sd_init_UART(void);

// Send data to the host
void sd_send_chr(const char chr);
void sd_send_string(const char *cstring);