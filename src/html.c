#include <stdio.h>
#include <string.h>
#include "time.h"
#include "lan.h"
#include "sensors.h"

extern void fill_buf_P(char **buf, const char * str);
extern void itoa(int n, char s[]);
#ifdef WITH_DS18B20
extern uint8_t DS18D20_SearchSensors( ROM roms[], int size );
#endif // WITH_DS18B20
char buffor[ 512 ];

/*----------------------------------------------------------
 *            Put your html source here...
 * NOTE: html-page source must have less than sizeof(buffor)!
 *----------------------------------------------------------*/

/**
 * html source page for URL: /configuration.html
 */
char* html_page_body_configuration(void){
	char *buf = buffor;
	memset( buf, 0, sizeof(buffor) );
	fill_buf_P(&buf,
			"<html>"
			"<style>"
				"table,th,td{"
					"border:1px solid black;"
					"border-collapse:collapse;"
					"padding:5px;"
					"text-align:center;"
					"width:40%;"
				"}"
			"</style>"
				);
	fill_buf_P(&buf,
			"<body>"
			"<h1 style=\"text-align:center\">STM32F100</h1>"
				);
	fill_buf_P(&buf,
			"<table align=center>"
				"<caption>CONFIGURATION</caption>"
				);
	sprintf(buf,
			"<tr><td>IP</td><td>%d.%d.%d.%d</td></tr>"
			"<tr><td>NETMASK</td><td>%d.%d.%d.%d</td></tr>"
			"<tr><td>GATEWAY</td><td>%d.%d.%d.%d</td></tr>"
#ifdef WITH_NTP
			"<tr><td>SERVER NTP</td><td>%d.%d.%d.%d</td></tr>"
			"<tr><td>SYSYEM TIME</td><td>%s</td></tr>"
#endif // WITH_NTP
														,
			INET_ADDR_A(ip_addr), INET_ADDR_B(ip_addr), INET_ADDR_C(ip_addr), INET_ADDR_D(ip_addr),
			INET_ADDR_A(ip_mask), INET_ADDR_B(ip_mask), INET_ADDR_C(ip_mask), INET_ADDR_D(ip_mask),
			INET_ADDR_A(ip_gateway), INET_ADDR_B(ip_gateway), INET_ADDR_C(ip_gateway), INET_ADDR_D(ip_gateway)
#ifdef WITH_NTP
			,INET_ADDR_A(SNTP_SERVER_ADDRESS), INET_ADDR_B(SNTP_SERVER_ADDRESS), INET_ADDR_C(SNTP_SERVER_ADDRESS), INET_ADDR_D(SNTP_SERVER_ADDRESS),
			asctime( gmtime( 0 /*time()*/ ) )
#endif // WITH_NTP
				);
	buf += strlen(buf);

	fill_buf_P(&buf,
			"</table>"
			"</body>"
			"</html>"
				);
	return buffor;
}

/**
 * html source page for URL: /sensors.html
 */
char* html_page_body_sensors(void){
	char *buf = buffor;
	memset( buf, 0, sizeof(buffor) );

	fill_buf_P(&buf,
			"<html>"
			"<style>"
				"table,th,td{"
					"border:1px solid black;"
					"border-collapse:collapse;"
					"padding:5px;"
					"text-align:center;"
					"width:40%;"
				"}"
			"</style>"
				);
	fill_buf_P(&buf,
			"<body>"
			"<h1 style=\"text-align:center\">STM32F100</h1>"
				);

#ifdef WITH_DS18B20
	fill_buf_P(&buf,
			"<table align=center>"
				);
	fill_buf_P(&buf,
				"<tr><td>ID</td><td>TEMPERATURE</td></tr>"
				);
	{
	int i;

	for (i = 0; i < owire_devices_count; i++) {

		sprintf(buf,
				"<tr><td>%d</td><td>%d</td></tr>", i, (int)(ds18b20_temps[i] * 100.0f));
		buf += strlen(buf);

	}
	}
	fill_buf_P(&buf,
			"</table>"
				);
#endif // WITH_BS18B20

	fill_buf_P(&buf,
			"</body>"
			"</html>"
				);

	return buffor;
}

/**
 * html source page for URL: /search.html
 */
char* html_page_body_search(void){
	char *buf = buffor;
	memset( buf, 0, sizeof(buffor) );

	fill_buf_P(&buf,
			"<html>"
			"<style>"
				"table,th,td{"
					"border:1px solid black;"
					"border-collapse:collapse;"
					"padding:5px;"
					"text-align:center;"
					"width:40%;"
				"}"
			"</style>"
				);
	fill_buf_P(&buf,
			"<body>"
			"<h1 style=\"text-align:center\">STM32F100</h1>"
				);

#ifdef WITH_DS18B20
	fill_buf_P(&buf,
			"<table align=center>"
			"<caption>1WIRE DEV/S</caption>"
				);
	{
	int i, owire_found_devices = 0;
	ROM roms[4];
	owire_found_devices = DS18D20_SearchSensors( roms, sizeof(roms)/sizeof(ROM) );

	for (i = 0; i < owire_found_devices; i++) {

		sprintf(buf,
				"<tr><td>%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x</td></tr>",
						roms[i].rom[0], roms[i].rom[1], roms[i].rom[2], roms[i].rom[3],
						roms[i].rom[4], roms[i].rom[5], roms[i].rom[6], roms[i].rom[7]);
		buf += strlen(buf);

	}
	}
	fill_buf_P(&buf,
			"</table>"
				);
#endif // WITH_BS18B20

	fill_buf_P(&buf,
			"</body>"
			"</html>"
				);

	return buffor;
}
