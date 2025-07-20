#include "main.h"

int length = 3;
max7219_t dev;
char direction[50];
dots snake_array[50];
TaskHandle_t xFood, xWalk, xRead;
extern int gpio_num_old;
int score_i = 0;

static char *TAG = "MQTT";
static void mqtt_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
int mqtt_send(const char *topic, const char *payload, bool retain);
static esp_mqtt_client_handle_t client;

void task_walk(void *pvParameter)
{
    task_setup(&dev);

    int hor = 0, ver = 2;

    snake_array[2] = zmijica[0][0];
    snake_array[1] = zmijica[0][1];
    snake_array[0] = zmijica[0][2];

    dots cross[16] = {};
    int num = 7;
    for (int i = 0; i < 8; i++)
    {
        cross[i] = zmijica[i][i];
    }
    for (int i = 7, j = 0; i >= 0; i--, j++)
    {
        cross[++num] = zmijica[i][j];
    }

    mqtt_send("esp_l/hrana", "GAME START", true);
    sprintf(direction, "down");

    while (1)
    {
        vTaskSuspend(xFood);

        // Moving each part of the snake to the previous one (FIFO / queue-like behavior)
        vTaskResume(xFood);
        vTaskDelay(pdMS_TO_TICKS(500)); // Giving time for interrupt to occur while the head is in position
        vTaskSuspend(xFood);

        for (int i = length; i >= 0; i--)
        {
            snake_array[i + 1] = snake_array[i];
        }

        // Snake movement
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
            // Default movement (down)
            ver++;
            if (ver == 8) ver = 0;
            snake_array[0] = zmijica[hor][ver];
        }

        for (int i = 1; i < length; i++)
        {
            if (snake_array[0].vertcal == snake_array[i].vertcal &&
                snake_array[0].horizontal == snake_array[i].horizontal)
            {
                gpio_num_old = 0;
                char message[30] = "END GAME";
                score_i = 0;
                mqtt_send("esp_l/hrana", message, true);
                strcpy(direction, "xx");

                while (strcmp(direction, "down") != 0)
                {
                    write_dots(&dev, 16, cross);
                    vTaskDelay(pdMS_TO_TICKS(500));
                    max7219_clear(&dev);
                    vTaskDelay(pdMS_TO_TICKS(500));
                }

                memset(snake_array, 0, length);
                snake_array[2] = zmijica[0][0];
                snake_array[1] = zmijica[0][1];
                snake_array[0] = zmijica[0][2];
                length = 3;
                sprintf(direction, "down");
                hor = 0;
                ver = 2;
                gpio_num_old = 0;
                write_dots(&dev, length, snake_array);
                strcpy(message, "GAME START");
                mqtt_send("esp_l/hrana", message, true);
            }
        }
    }
}

void food(void *pvParameter)
{
    int i = dot_rand();
    int j = dot_rand();
    char score[50];

    while (1)
    {
        snake_array[length] = zmijica[j][i];
        write_dots(&dev, length + 1, snake_array);
        vTaskDelay(pdMS_TO_TICKS(150));
        write_dots(&dev, length, snake_array);
        vTaskDelay(pdMS_TO_TICKS(150));

        if ((snake_array[0].horizontal == snake_array[length].horizontal) &&
            (snake_array[0].vertcal == snake_array[length].vertcal))
        {
            score_i++;
            sprintf(score, "%d", score_i);
            mqtt_send("esp_l/hrana", score, true);
            length++;
            i = dot_rand();
            j = dot_rand();

            for (int ind = 0; ind < length; ind++)
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

void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_connect_init();
    ESP_ERROR_CHECK(wifi_connect_sta("POCO F6", "12345678", 10000));

    esp_mqtt_client_config_t esp_mqtt_client_config = {
        .broker.address.uri = "mqtt://broker.hivemq.com:1883",
        .session.last_will = {
            .topic = "esp_l/on-chip-death",
            .msg = "I died ☠️",
            .msg_len = strlen("I died ☠️"),
            .qos = 1,
        }
    };

    client = esp_mqtt_client_init(&esp_mqtt_client_config);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    xTaskCreatePinnedToCore(food, "food", configMINIMAL_STACK_SIZE * 2, NULL, 4, &xFood, 0);
    xTaskCreatePinnedToCore(task_walk, "task_walk", configMINIMAL_STACK_SIZE * 5, NULL, 5, &xWalk, 0);
}

static void mqtt_event_handler(void *event_handler_arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        esp_mqtt_client_subscribe(client, "esp_l/smjer", 1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED");
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED");
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED");
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("topic: %.*s\n", event->topic_len, event->topic);
        printf("message: %.*s\n", event->data_len, event->data);

        sprintf(direction, "%.*s", event->data_len, event->data);

        if (strncmp(event->topic, "esp_l/smjer", strlen("esp_l/smjer")) == 0)
        {
            printf("received direction: %.*s\n", event->data_len, event->data);
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "ERROR %s", strerror(event->error_handle->esp_transport_sock_errno));
        break;
    default:
        break;
    }
}

int mqtt_send(const char *topic, const char *payload, bool retain)
{
    return esp_mqtt_client_publish(client, topic, payload, strlen(payload), 1, retain);
}
