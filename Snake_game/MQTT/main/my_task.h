
#ifndef __MY_TASK_H__
#define __MY_TASK_H__

#include "main.h"

void task_setup(max7219_t *dev);
void write_dots(max7219_t *dev,int lenght,dots *sequence );
int dot_rand(void);

#endif