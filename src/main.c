/* Include core modules */
#include "stm32fxxx_hal.h"
/* Include my libraries here */
#include "defines.h"
#include "config.h"
#include "tm_stm32_delay.h"
#include "tm_stm32_usart.h"
#ifdef WITH_DS18B20
#include "tm_stm32_ds18b20.h"
#include "tm_stm32_onewire.h"
#endif // WITH_DS18B20
#ifdef WITH_LAN
#include "lan.h"
#endif // WITH_LAN
#ifdef WITH_MPL115A2
#include "stm32_mpl115a2.h"
#endif // WITH_MPL115A2

__attribute__ ((used)) int putchar(int ch) {
	/* Send over debug USART */
	TM_USART_Putc(USART1, ch);

	/* Return OK */
	return ch;
}

#include "os_compat.h"
#include "sensors.h"

#define PROJECT_NAME	"IOT_NODE"
#define AUTHOR			"Pablo <w_pawel74@tlen.pl>"

#ifdef WITH_DS18B20
#define DS18B20_SCAN
float ds18b20_temps[EXPECTING_SENSORS];
ROM roms[EXPECTING_SENSORS] = {
#ifndef DS18B20_SCAN
	{ .rom = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },
#endif // DS18B20_SCAN
};
uint8_t owire_devices_count =
#ifndef DS18B20_SCAN
		EXPECTING_SENSORS
#else
		0
#endif // DS18B20_SCAN
		;
static TM_OneWire_t OneWire1;
static uint32_t ds18b20_read_time = 0;

int DS18D20_SearchSensors( ROM roms[], int size ){
	owire_devices_count = 0;
	uint8_t devices = TM_OneWire_First(&OneWire1);
	while (devices) {
		/* Increase counter */
		owire_devices_count++;

		/* Get full ROM value, 8 bytes, give location of first byte where to save */
		TM_OneWire_GetFullROM(&OneWire1, roms[owire_devices_count - 1].rom);

		/* Get next device */
		devices = TM_OneWire_Next(&OneWire1);
	}
	return owire_devices_count;
}

void DS18D20_ReadTemp(void){
	int i;
	/* Start temperature conversion on all devices on one bus */
	TM_DS18B20_StartAll(&OneWire1);

	/* Wait until all are done on one onewire port */
	while (!TM_DS18B20_AllDone(&OneWire1));

	/* Read temperature from each device separatelly */
	for (i = 0; i < owire_devices_count; i++) {
		/* Read temperature from ROM address and store it to temps variable */
		if (TM_DS18B20_Read(&OneWire1, roms[i].rom, &ds18b20_temps[i])) {
			/* Print temperature */
			_D(("[DS18B20] ROM>%d: Temp %f\n", i, ds18b20_temps[i]));
		} else {
			/* Reading error */
			_D(("[DS18B20] Reading error ROM>%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
									roms[i].rom[0], roms[i].rom[1], roms[i].rom[2], roms[i].rom[3],
									roms[i].rom[4], roms[i].rom[5], roms[i].rom[6], roms[i].rom[7]));
		}
	}
}

#endif // WITH_DS18B20

int main(void) {

	/* Init system */
	TM_RCC_InitSystem();

	TM_USART_Init(USART1, TM_USART_PinsPack_1, 115200);

	_D(("---------------------------------------------\n"));
	_D((" Project: %s\n", PROJECT_NAME));
	_D((" Author: %s\n", AUTHOR));
	_D((" Release date: %s %s\n", __DATE__, __TIME__));
	_D(("---------------------------------------------\n"));

	/* Init HAL layer */
	HAL_Init();

	/* Init delay */
	TM_DELAY_Init();

#ifdef WITH_MPL115A2
	PW_MPL115A2_t mpl = {
			.I2Cx = I2C1,
#ifdef MPL115A2_USE_RST
			.GPIOx_RST = GPIOA,
			.GPIO_Pin_RST = GPIO_PIN_1,
#endif // MPL115A2_USE_RST
#ifdef MPL115A2_USE_SHDN
			.GPIOx_SHDN = GPIOA,
			.GPIO_Pin_SHDN = GPIO_PIN_2,
#endif // MPL115A2_USE_SHDN
	};
	PW_MPL115A2_Init( &mpl );
#endif // WITH_MPL115A2

#ifdef WITH_LAN
	// eth initialize
	LAN_init();
#endif // WITH_LAN

#ifdef WITH_LEDS
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_SetBits(GPIOA, GPIO_Pin_11 | GPIO_Pin_12 );
#endif // WITH_LEDS

#ifdef WITH_DS18B20
	TM_OneWire_Init( &OneWire1, GPIOC, GPIO_Pin_13 );

#ifdef DS18B20_SCAN
	owire_devices_count = DS18D20_SearchSensors( roms, sizeof(roms)/sizeof(ROM) );
#endif // DS18B20_SCAN
	int i;
	/* Go through all connected devices and set resolution to 12bits */
	for (i = 0; i < owire_devices_count; i++) {
		/* Set resolution to 12bits */
		TM_DS18B20_SetResolution(&OneWire1, roms[i].rom, TM_DS18B20_Resolution_12bits);
	}

	for (i = 0; i < owire_devices_count; i++) {

		_D(("[DS18B20] ROM> %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x ... %s\n",
						roms[i].rom[0], roms[i].rom[1], roms[i].rom[2], roms[i].rom[3],
						roms[i].rom[4], roms[i].rom[5], roms[i].rom[6], roms[i].rom[7],
						TM_DS18B20_Is( roms[i].rom ) ? "online": "offline"));
	}
#endif // WITH_DS18B20

	while(1) {

#ifdef WITH_LAN
		LAN_poll();
#endif // WITH_LAN

#ifdef WITH_DS18B20
		if( HAL_GetTick() > ds18b20_read_time ){
			DS18D20_ReadTemp();
			ds18b20_read_time = HAL_GetTick() + 10 * 1000;
		}
#endif // WITH_DS18B20

	}

	return 0;
}
