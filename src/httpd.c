#include <stdio.h>
#include "config.h"
#include "os_compat.h"
#include "httpd.h"

#define len(some) (sizeof(some)/sizeof(some[0]))

const char  http_200[] = "HTTP/1.0 200 OK\r\n";
const char http_404[] = "HTTP/1.0 404 Not Found\r\n";
const char http_server[] = "Server: "HTTPD_NAME"\r\n";
const char http_content_type[] = "Content-Type: ";
const char http_content_length[] = "Content-Length: ";
const char http_connection_close[] = "Connection: close\r\n";
const char  http_linebreak[] = "\r\n";
const char  http_header_end[] = "\r\n\r\n";
const char http_not_found[] = "<h1>404 - Not Found</h1>";

const char http_text_plain[] = "text/plain";
const char http_text_html[] = "text/html";
const char http_text_js[] = "text/javascript";
const char  http_text_css[] = "text/css";
const char http_image_gif[] = "image/gif";
const char http_image_png[] = "image/png";
const char  http_image_jpeg[] = "image/jpeg";

const char http_ext_txt[] = "txt";
const char  http_ext_htm[] = "htm";
const char  http_ext_html[] = "html";
const char  http_ext_js[] = "js";
const char  http_ext_css[] = "css";
const char  http_ext_gif[] = "gif";
const char http_ext_png[] = "png";
const char  http_ext_jpg[] = "jpg";
const char  http_ext_jpeg[] = "jpeg";

const char *mime_type_table[][2] =
{
	{http_ext_txt, http_text_plain},
	{http_ext_htm, http_text_html},
	{http_ext_html, http_text_html},
	{http_ext_js, http_text_js},
	{http_ext_css, http_text_css},
	{http_ext_gif, http_image_gif},
	{http_ext_png, http_image_png},
	{http_ext_jpg, http_image_jpeg},
	{http_ext_jpeg, http_image_jpeg}
};

extern char* html_page_body_configuration(void);
extern char* html_page_body_sensors(void);
extern char* html_page_body_search(void);

//****************************************************
// reverse:
void reverse(char s[])
{
    int i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}
// itoa
 void itoa(int n, char s[])
 {
     int i, sign;

     if ((sign = n) < 0)  /* sign */
         n = -n;          /* make positive n */
     i = 0;
     do {       /* generate reverse */
         s[i++] = n % 10 + '0';   /* next digit */
     } while ((n /= 10) > 0);     /* delete */
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
 }

 //*********************************************






// httpd state
httpd_state_t httpd_pool[TCP_MAX_CONNECTIONS];

void fill_buf(char **buf, char *str);
void fill_buf_P(char **buf, const char * str);

// get mime type from filename extension
const char *httpd_get_mime_type(char *url)
{
	const char *t_ext, *t_type;
	char *ext;
	uint8_t i;

	if((ext = strrchr(url, '.')))
	{
		ext++;
	//	strlwr(ext);

		for(i = 0; i < len(mime_type_table); ++i)
		{
			t_ext = (void*)(mime_type_table[i] + 0);

			if(!strcmp(ext, t_ext))
			{
				t_type = (void*)(mime_type_table[i] + 1);
				return t_type;
			}
		}
	}
	return 0;
}



// processing HTTP request
void httpd_request(httpd_state_t *st, char *url)
{
	// index file requested?
	if(!strcmp(url, "/"))
		url = HTTPD_INDEX_FILE;

	_D(("HTTP: URL %s\n", url));

	st->statuscode = 2;
	if( strcmp(url, "/configuration.html") == 0 ){
		st->data = html_page_body_configuration();
		st->numbytes = strlen( st->data );
		st->type = httpd_get_mime_type(url);
		st->file_opened = 0;
		return;
	} else if( strcmp(url, "/sensors.html") == 0 ){
		st->data = html_page_body_sensors();
		st->numbytes = strlen( st->data );
		st->type = httpd_get_mime_type(url);
		st->file_opened = 0;
		return;
	} else if( strcmp(url, "/search.html") == 0 ){
		st->data = html_page_body_search();
		st->numbytes = strlen( st->data );
		st->type = httpd_get_mime_type(url);
		st->file_opened = 0;
		return;
	}

#ifdef MMC_SUPPORT
	if(!f_open(&(st->fs), url, FA_READ))
	{
		st->statuscode = 2;
		st->data = 0;
		st->numbytes = st->fs.fsize;
		st->type = httpd_get_mime_type(url);
		st->file_opened = 1;
	}
	else
#endif // MMC_SUPPORT
	{
		st->statuscode = 4;
		st->data = http_not_found;
		st->numbytes = sizeof(http_not_found)-1;
		st->type = http_text_html;
	}

#ifdef WITH_TCP_REXMIT
	// save state
	st->statuscode_saved = st->statuscode;
	st->data_saved = st->data;
	st->numbytes_saved = st->numbytes;
#endif
}




// prepare HTTP reply header
void httpd_header(httpd_state_t *st, char **buf)
{
	char str[16];

	// Status
	if(st->statuscode == 2)
		fill_buf_P(buf, http_200);
	else
		fill_buf_P(buf, http_404);

	// Content-Type
	if(st->type)
	{
		fill_buf_P(buf, http_content_type);
		fill_buf_P(buf, st->type);
		fill_buf_P(buf, http_linebreak);
	}

	// Content-Length
	if(st->numbytes)
	{
		itoa(st->numbytes, str);
		fill_buf_P(buf, http_content_length);
		fill_buf(buf, str);
		fill_buf_P(buf, http_linebreak);
	}

	// Server
	fill_buf_P(buf, http_server);

	// Connection: close
	fill_buf_P(buf, http_connection_close);

	// Header end
	fill_buf_P(buf, http_linebreak);
}



// accepts incoming connections
ATTR_USED uint8_t LAN_Callback_TCPListen(uint8_t id, eth_frame_t *frame)
{
	ip_packet_t *ip = (void*)(frame->data);
	tcp_packet_t *tcp = (void*)(ip->data);

	// accept connections to port 80
	return (tcp->to_port == HTTPD_PORT);
}


// upstream callback
ATTR_USED void LAN_Callback_TCPRead(uint8_t id, eth_frame_t *frame, uint8_t re)
{
	httpd_state_t *st = httpd_pool + id;
	ip_packet_t *ip = (void*)(frame->data);
	tcp_packet_t *tcp = (void*)(ip->data);
	char *buf = (void*)(tcp->data), *bufptr;
	UINT blocklen;
	uint8_t i, close = 0;
	uint16_t sectorbytes;
	uint8_t options;

	// Connection opened
	if(st->status == HTTPD_CLOSED)
	{
		st->status = HTTPD_INIT;
	}

	// Sending data
	else if(st->status == HTTPD_WRITE_DATA)
	{

#ifdef WITH_TCP_REXMIT
		if(re)
		{
			// load previous state
			st->statuscode = st->statuscode_saved;
			st->numbytes = st->numbytes_saved;
			st->data = st->data_saved;
#ifdef MMC_SUPPORT
			if(st->file_opened)
				f_lseek(&(st->fs), st->fs.fsize - st->numbytes);
#endif // MMC_SUPPORT
		}
		else
		{
			// save state
			st->statuscode_saved = st->statuscode;
			st->numbytes_saved = st->numbytes;
			st->data_saved = st->data;
		}
#endif

		// Send bulk of packets
		for(i = HTTPD_PACKET_BULK; i; --i)
		{
			blocklen = HTTPD_MAX_BLOCK;
			bufptr = buf;

			// Put HTTP header to buffer
			if(st->statuscode != 0)
			{
				httpd_header(st, &bufptr);
				blocklen -= bufptr - buf;
				st->statuscode = 0;
			}

			// Send up to 512 bytes (-header)
			if(st->numbytes < blocklen)
				blocklen = st->numbytes;

			// Copy data to buffer...
			if(st->data)
			{
				memcpy(bufptr, st->data, blocklen);
				bufptr += blocklen;

				st->data += blocklen;
				st->numbytes -= blocklen;
			}

			// ... or Read file to buffer
			else if(st->file_opened)
			{
				// align read to sector boundary
				sectorbytes = 512 - ((uint16_t)(st->fs.fptr) & 0x1ff);
				if(blocklen > sectorbytes)
					blocklen = sectorbytes;
#ifdef MMC_SUPPORT
				f_read(&(st->fs), bufptr, blocklen, &blocklen);
#endif // MMC_SUPPORT

				bufptr += blocklen;

				st->numbytes -= blocklen;
			}

			// Send packet
			if(!st->numbytes)
				options = TCP_OPTION_CLOSE;
			else if(i == 1)
				options = TCP_OPTION_PUSH;
			else
				options = 0;

			LAN_TCPSend(id, frame, bufptr - buf, options);

			if(close)
				break;
		}
	}
}


// downstream callback
ATTR_USED void LAN_Callback_TCPWrite(uint8_t id, eth_frame_t *frame, uint16_t len)
{
	httpd_state_t *st = httpd_pool + id;
	ip_packet_t *ip = (void*)(frame->data);
	tcp_packet_t *tcp = (void*)(ip->data);
	char *request = (void*)tcp_get_data(tcp);
	char *url, *p;

	request[len] = 0;

	// just connected?
	if(st->status == HTTPD_INIT)
	{
		// extract URL from request header
		url = request + 4;
		if( (!memcmp(request, "GET ", 4)) &&
			((p = strchr(url, ' '))) )
		{
			*(p++) = 0;

			// process URL request
			httpd_request(st, url);

			// skip other fields
			if(strstr(p, http_header_end))
				st->status = HTTPD_WRITE_DATA;
			else
				st->status = HTTPD_READ_HEADER;
		}
	}

	// receiving HTTP header?
	else if(st->status == HTTPD_READ_HEADER)
	{
		// skip all fields
		if(strstr(request, http_header_end))
			st->status = HTTPD_WRITE_DATA;
	}
}

// connection closing handler
ATTR_USED void LAN_Callback_TCPClosed(uint8_t id, uint8_t hard)
{
	httpd_state_t *st = httpd_pool + id;

	if(st->file_opened)
	{
#ifdef MCC_SUPPORT
		f_close(&(st->fs));
#endif // MMC_SUPPORT
		st->file_opened = 0;
	}
	st->status = HTTPD_CLOSED;
}


// utilities
void fill_buf(char **buf, char *str)
{
	uint16_t len = 0;
	char *p = *buf;

	while(*str)
		p[len++] = *(str++);
	*buf += len;
}
//const type
void fill_buf_P(char **buf, const char * str)
{
	uint16_t len = 0;
	char *p = *buf;

	while(*str)
		p[len++] = *(str++);
	*buf += len;
}
