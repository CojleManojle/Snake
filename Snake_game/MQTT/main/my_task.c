#include "my_task.h"

void task_setup(max7219_t *dev)
{
    spi_bus_config_t cfg = {
        .mosi_io_num = MOSI_PIN,
        .miso_io_num = -1,
        .sclk_io_num = CLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
        .flags = 0
    };

    ESP_ERROR_CHECK(spi_bus_initialize(HOST, &cfg, 1));

    dev->cascade_size = CASCADE_SIZE;
    dev->digits = 0;
    dev->mirrored = true;

    ESP_ERROR_CHECK(max7219_init_desc(dev, HOST, MAX7219_MAX_CLOCK_SPEED_HZ, CS_PIN));
    ESP_ERROR_CHECK(max7219_init(dev)); // Initial matrix setup
    init_dot();

    ESP_LOGI("GAME-INFO", "GAME STARTING\n");
}

void write_dots(max7219_t *dev, int length, dots *sequence)
{
    /* 
        'horizontal' tells us which horizontal part we want (given by a number),
        while 'vertical' uses bits to indicate which LEDs we want to light up.

        ****************************************************
        The idea is to prepare a buffer matrix and add items into it,
        then go through it row by row (horizontal) and OR the vertical bits as long as the
        horizontal is the same. When a new horizontal appears, we write the previous value.
    */

    uint8_t vertical_array[8] = {};
    int vertical = 0;
    int old_horizontal = 0;

    max7219_clear(dev);

    int i = length - 1;
    while (i >= 0)
    {
        vertical |= sequence[i].vertcal;
        old_horizontal = sequence[i].horizontal;
        i--;

        if (i != -1)
        {
            if (sequence[i].horizontal != old_horizontal)
            {
                vertical_array[old_horizontal] |= vertical; // store vertical bits to preserve values
                max7219_set_digit(dev, old_horizontal, vertical_array[old_horizontal]);
                vertical = 0;
            }
        }
        else
        {
            // This is necessary because when we reach -1, we get garbage memory
            // which sometimes caused steps to be skipped due to matching "junk" values.
            vertical_array[old_horizontal] |= vertical;
            max7219_set_digit(dev, old_horizontal, vertical_array[old_horizontal]);
            vertical = 0;
        }
    }
}

int dot_rand(void)
{
    int random = esp_random();
    int positiveNumber = abs(random);
    int diceNumber = (positiveNumber % 7); // generates number in [0,6]
    return diceNumber;
}
