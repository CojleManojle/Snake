#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include <esp_idf_version.h>
#include "max7219.h"
#include "esp_random.h"
#include "math.h"
#include "nvs_flash.h"
#include "wifi_connect.h"
#include "mqtt_client.h"


#include "my_task.h"
#include "gpio.h"

#ifndef APP_CPU_NUM
#define APP_CPU_NUM PRO_CPU_NUM
#endif
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(4, 0, 0)
#define HOST    HSPI_HOST
#else
#define HOST    SPI2_HOST
#endif

#define CASCADE_SIZE 1
#define MOSI_PIN 23
#define CS_PIN 5
#define CLK_PIN 18


extern dots zmijica[8][8];



#endif