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
     * 'horizontal' indicates which horizontal row we want, given by an index.
     * 'vertical' is specified using bits that we want to add.
     *
     * ****************************************************
     * The idea is that we create a matrix and add it to an array that we loop through sequentially.
     * First we OR the vertical bits together while the horizontal remains the same.
     * When we detect a new horizontal row, we write the previously gathered vertical value.
     */

    uint8_t vertical_array[8] = {};
    int vertical = 0;
    int previous_horizontal = 0;

    max7219_clear(dev);

    int i = length - 1;
    while (i >= 0)
    {
        vertical |= sequence[i].vertcal;
        previous_horizontal = sequence[i].horizontal;
        i--;

        if (i != -1)
        {
            if (sequence[i].horizontal != previous_horizontal)
            {
                vertical_array[previous_horizontal] |= vertical; // Used to store previous vertical values
                // In case elements are not adjacent
                max7219_set_digit(dev, previous_horizontal, vertical_array[previous_horizontal]);
                vertical = 0;
            }
        }
        else
        {
            // This is necessary because when we reach index -1, we can get garbage memory
            // which accidentally caused the loop to skip steps if that garbage happened to match
            // a valid value. This prevented the loop from properly finishing.
            vertical_array[previous_horizontal] |= vertical;
            max7219_set_digit(dev, previous_horizontal, vertical_array[previous_horizontal]);
            vertical = 0;
        }
    }
}

int dot_rand(void)
{
    int random = esp_random();
    int positive = abs(random);
    int result = positive % 7;
    return result;
}
