#include "gpio.h"

int gpio_num_old = 0 ; 

static void IRAM_ATTR gpio_isr_handler(void* arg)
{   
   // vTaskDelay(pdMS_TO_TICKS(10));
    uint32_t gpio_num = (uint32_t) arg;
    if(gpio_num_old != gpio_num)
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
    gpio_num_old = gpio_num;
}





void gpio_setup()
{

    gpio_config_t io_conf = {};

    //zero-initialize the config structure.
    //gpio_config_t io_conf = {};
    //enacle rising edge interrupt
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //bit mask of the pins that you want to set,e.g.GPIO36/39/32/33
    io_conf.pin_bit_mask = GPIO_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    //enable pull-up mode
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

   //create a queue to handle gpio event from isr

     //install gpio isr service
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO1, gpio_isr_handler, (void*) GPIO1);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO2, gpio_isr_handler, (void*) GPIO2);
    //  hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO3, gpio_isr_handler, (void*) GPIO3);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO4, gpio_isr_handler, (void*) GPIO4);

}


