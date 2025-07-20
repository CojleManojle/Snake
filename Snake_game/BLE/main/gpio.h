
#ifndef __GPIO_H__
#define __GPIO_H__

#include "main.h"

#define GPIO1 32
#define GPIO2 33
#define GPIO3 22
#define GPIO4 21

#define GPIO_PIN_SEL ((1ULL<<GPIO1) | (1ULL<<GPIO2) | (1ULL<<GPIO3) | (1ULL<<GPIO4) )


#define GPIO_OUTPUT_IO_0    22
#define GPIO_OUTPUT_IO_1    21
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))

void gpio_setup(void);

extern QueueHandle_t gpio_evt_queue;

#endif