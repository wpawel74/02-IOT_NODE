#pragma once

#include <stdlib.h>
#include <string.h>
#include "ff/ff.h"
#include "lan.h"

#define HTTPD_PORT			htons(80)
#define HTTPD_PACKET_BULK	10
#define HTTPD_MAX_BLOCK		512
#define HTTPD_NAME			"STM32F100"
#define HTTPD_INDEX_FILE	"/index.htm"

typedef enum httpd_state_code {
	HTTPD_CLOSED,
	HTTPD_INIT,
	HTTPD_READ_HEADER,
	HTTPD_WRITE_DATA
} httpd_state_code_t;

#pragma pack(push, 1)
typedef struct httpd_state {
	httpd_state_code_t status;

	// header
	uint8_t statuscode;				// HTTP status code
	const char *type;			// content type
	uint32_t numbytes;				// content length

	// data
	const char *data;			// progmem data
	
	uint8_t file_opened;
	FIL fs;							// file

	// saved state
#ifdef WITH_TCP_REXMIT
	uint8_t statuscode_saved;		// status code
	uint32_t numbytes_saved;		// content length

	const char *data_saved;	// promem data
#endif
} httpd_state_t;

#pragma pack(pop)

