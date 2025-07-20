#include "main.h"

#define DEVICE_NAME "MY BLE DEVICE"
#define DEVICE_INFO_SERVICE 0x180A
#define MANUFACTURER_NAME 0x2A29

#define CUSTOM_SERVICE_UUID      0x1234
#define CHARACTERISTIC_UUID      0x5678

int snake_length = 3;
max7219_t dev;

char direction[50];
dots snake_array[50];
TaskHandle_t xFood, xWalk, xRead;
extern int gpio_num_old;
int score_value = 0;
uint8_t ble_addr_type;
uint16_t conn_hdl;
uint16_t batt_char_attr_hdl;
int start = 0;

void ble_app_advertise(void);
int step = 0;

void task_walk(void *pvParameter)
{
    task_setup(&dev);

    int hor = 0, ver = 2;

    snake_array[2] = zmijica[0][0];
    snake_array[1] = zmijica[0][1];
    snake_array[0] = zmijica[0][2];

    dots cross_pattern[16] = {};
    int num = 7;
    for (int i = 0; i < 8; i++)
    {
        cross_pattern[i] = zmijica[i][i];
    }
    for (int i = 7, j = 0; i >= 0; i--, j++)
    {
        cross_pattern[++num] = zmijica[i][j];
    }

    sprintf(direction, "down");

    while (1)
    {
        vTaskSuspend(xFood);
        // move each element one place back, making a new head and removing the tail; retains the rest of the snake
        // behaves like a FIFO queue
        vTaskResume(xFood);
        vTaskDelay(pdMS_TO_TICKS(500)); // wait to allow interrupt at the position where the head reaches
        vTaskSuspend(xFood);

        for (int i = snake_length; i >= 0; i--)
        {
            snake_array[i + 1] = snake_array[i];
        }

        // snake movement
        if (strcmp(direction, "down") == 0)
        {
            ver++;
            if (ver == 8) ver = 0;
            snake_array[0] = zmijica[hor][ver];
        }
        else if (strcmp(direction, "left") == 0)
        {
            hor++;
            if (hor == 8) hor = 0;
            snake_array[0] = zmijica[hor][ver];
        }
        else if (strcmp(direction, "up") == 0)
        {
            ver--;
            if (ver == -1) ver = 7;
            snake_array[0] = zmijica[hor][ver];
        }
        else if (strcmp(direction, "right") == 0)
        {
            hor--;
            if (hor == -1) hor = 7;
            snake_array[0] = zmijica[hor][ver];
        }
        else
        {
            ver++;
            if (ver == 8) ver = 0;
            snake_array[0] = zmijica[hor][ver];
        }

        for (int i = 1; i < snake_length; i++)
        {
            if (snake_array[0].vertcal == snake_array[i].vertcal &&
                snake_array[0].horizontal == snake_array[i].horizontal)
            {
                gpio_num_old = 0;
                char message[30] = "END GAME";
                score_value = 0;
                strcpy(direction, "xx");

                while (strcmp(direction, "down") != 0)
                {
                    write_dots(&dev, 16, cross_pattern);
                    vTaskDelay(pdMS_TO_TICKS(500));
                    max7219_clear(&dev);
                    vTaskDelay(pdMS_TO_TICKS(500));
                }

                memset(snake_array, 0, snake_length);
                snake_array[2] = zmijica[0][0];
                snake_array[1] = zmijica[0][1];
                snake_array[0] = zmijica[0][2];
                snake_length = 3;
                sprintf(direction, "down");
                hor = 0;
                ver = 2;
                gpio_num_old = 0;
                write_dots(&dev, snake_length, snake_array);
                strcpy(message, "GAME START");
            }
        }
    }
}

void food(void *pvParameter)
{
    int i = 0, j = 0;
    i = dot_rand();
    j = dot_rand();
    char score[50];

    while (1)
    {
        snake_array[snake_length] = zmijica[j][i];
        write_dots(&dev, snake_length + 1, snake_array);
        vTaskDelay(pdMS_TO_TICKS(150));
        write_dots(&dev, snake_length, snake_array);
        vTaskDelay(pdMS_TO_TICKS(150));

        if ((snake_array[0].horizontal == snake_array[snake_length].horizontal) &&
            (snake_array[0].vertcal == snake_array[snake_length].vertcal))
        {
            score_value++;
            if (1 == start)
            {
                struct os_mbuf *om = ble_hs_mbuf_from_flat(&score_value, sizeof(score_value));
                ble_gattc_notify_custom(conn_hdl, batt_char_attr_hdl, om);
            }

            snake_length++;
            i = dot_rand();
            j = dot_rand();

            for (int ind = 0; ind < snake_length; ind++)
            {
                if (zmijica[j][i].horizontal == snake_array[ind].horizontal &&
                    zmijica[j][i].vertcal == snake_array[ind].vertcal)
                {
                    i = dot_rand();
                    j = dot_rand();
                    ind = 0;
                }
            }
        }
    }
}

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI("GAP", "BLE_GAP_EVENT_CONNECT %s", event->connect.status == 0 ? "OK" : "Failed");
        if (event->connect.status != 0)
        {
            ble_app_advertise();
        }
        conn_hdl = event->connect.conn_handle;
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI("GAP", "BLE_GAP_EVENT_DISCONNECT");
        ble_app_advertise();
        break;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI("GAP", "BLE_GAP_EVENT_ADV_COMPLETE");
        ble_app_advertise();
        break;

    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI("GAP", "BLE_GAP_EVENT_SUBSCRIBE");
        if (event->subscribe.attr_handle == batt_char_attr_hdl)
        {
            start = 1;
        }
        break;

    default:
        break;
    }
    return 0;
}

char temp_buf[20];

static int device_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    printf("incoming message: %.*s\n", ctxt->om->om_len, ctxt->om->om_data);
    strncpy(direction, (const char *)ctxt->om->om_data, ctxt->om->om_len);
    direction[ctxt->om->om_len] = '\0';
    printf("direction: %s\n", direction);
    return 0;
}

static int device_info(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    static uint8_t level = 50;
    os_mbuf_append(ctxt->om, &level, sizeof(level));
    return 0;
}

static int battery_level_descriptor(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    return 0;
}

static const struct ble_gatt_svc_def gat_svcs[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID128_DECLARE(0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff),
     .characteristics = (struct ble_gatt_chr_def[]){
         {.uuid = BLE_UUID128_DECLARE(0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                      0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xAA),
          .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
          .access_cb = device_info,
          .val_handle = &batt_char_attr_hdl,
          .descriptors = (struct ble_gatt_dsc_def[]){
              {
                  .uuid = BLE_UUID128_DECLARE(0x00, 0x11, 
