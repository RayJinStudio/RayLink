#include <stdlib.h>
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tusb.h"
#include "usbInit.h"
#include "cdcDevice.hpp"
#include "vendorDevice.hpp"
#include "UartDrive.hpp"
#include "dapInterface.h"

static const char *TAG = "main";

/*
这看起来没有用，但千万不能删，删了就会出问题
猜测是.o文件没有被调用，链接时被优化掉了
*/
void usbCallback()
{
    vendorCallback(); //这个千万不能删，删了vendor的回调就没了
    cdcCallback();    //这个可以删，为了美观就留着了
}

void tudTask(void *param)
{
    while (1)
    {
        tud_task();
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Device initialization");

    UartInit();
    dapSetUp();
    usbInit();

    xTaskCreate(tudTask, "tudTask", 4096, NULL, 5, NULL);
    xTaskCreate(cdcSendTask, "cdcSenderTask", 4096, NULL, 5, NULL);
    xTaskCreate(uartTask, "uartTask", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Device initialization DONE");
}
