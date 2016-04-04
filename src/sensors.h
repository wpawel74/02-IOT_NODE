#ifndef SENSORS_H
#define SENSORS_H
#include "config.h"

#ifdef WITH_DS18B20
/* How many sensors we are expecting on 1wire bus? */
#define EXPECTING_SENSORS    4

typedef struct OWIRE_ROM {
	uint8_t rom[8];
} ROM;

extern ROM device[EXPECTING_SENSORS];
extern float ds18b20_temps[EXPECTING_SENSORS];
extern uint8_t owire_devices_count;
#endif // WITH_DS18B20

#endif // SENSORS_H
