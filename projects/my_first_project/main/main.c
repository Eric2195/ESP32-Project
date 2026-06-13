#include <stdio.h>                  // C标准库
#include "freertos/FreeRTOS.h"      // FreeRTOS核心
#include "freertos/task.h"          // 任务管理API
#include "esp_log.h"                // 日志系统
#include "driver/gpio.h"            // GPIO驱动

static const char *TAG = "MAIN";    // 日志标签

#define LED_GPIO GPIO_NUM_8         // LED接在GPIO8

void blink_task(void *pvParameter)  // 闪烁任务
{
    int led_state = 0;              // LED当前状态，0灭1亮
    while (1) {
        led_state = !led_state;     // 翻转状态
        gpio_set_level(LED_GPIO, led_state);           // 设置GPIO电平
        ESP_LOGI(TAG, "LED %s", led_state ? "ON" : "OFF");
        vTaskDelay(pdMS_TO_TICKS(1000));                // 每秒闪一次
    }
}

void app_main(void)                 // 程序入口
{
    // 配置GPIO8为输出模式
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),    // 哪个引脚
        .mode = GPIO_MODE_OUTPUT,               // 输出模式
        .pull_up_en = GPIO_PULLUP_DISABLE,      // 不用上拉
        .pull_down_en = GPIO_PULLDOWN_DISABLE,  // 不用下拉
        .intr_type = GPIO_INTR_DISABLE          // 不用中断
    };
    gpio_config(&io_conf);                      // 应用配置

    ESP_LOGI(TAG, "Project started!");
    xTaskCreate(&blink_task,        // 任务函数
                "blink_task",       // 任务名
                2048,               // 栈大小
                NULL,               // 传参
                5,                  // 优先级
                NULL);              // 句柄
}
