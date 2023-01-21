#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "tusb_console.h"
#include "tusb_cdc_acm.h"

static const int UART_RX_BUF_SIZE = 1024;
static const int uart_num = UART_NUM_2;
#define TXD_PIN (GPIO_NUM_6)
#define RXD_PIN (GPIO_NUM_7)


QueueHandle_t uart_queue;
static RingbufHandle_t usb_sendbuf;
static SemaphoreHandle_t usb_tx_requested = NULL;
static SemaphoreHandle_t usb_tx_done = NULL;
static bool serial_read_enabled = false;
static const char *TAG = "UartDrive";

void serial_set(const bool enable)
{
    serial_read_enabled = enable;
}

bool uartSetBaudrate(const int baud)
{
    return uart_set_baudrate(uart_num, baud) == ESP_OK;
}

void UartInit(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_APB,
    };
    
    uart_driver_install(uart_num, UART_RX_BUF_SIZE, 0, 20, &uart_queue, 0);
    uart_param_config(uart_num, &uart_config);
    uart_set_pin(uart_num, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    usb_sendbuf = xRingbufferCreate(UART_RX_BUF_SIZE * 2, RINGBUF_TYPE_BYTEBUF);
    usb_tx_done = xSemaphoreCreateBinary();
    usb_tx_requested = xSemaphoreCreateBinary();
}

int uartSendData(const uint8_t* data, int len)
{
    const int txBytes = uart_write_bytes(uart_num, data, len);
    return txBytes;
}
 
void uartTask(void *pvParameters)
{
    uart_event_t uart_event;
    uint8_t data[UART_RX_BUF_SIZE];
    while(1)
    {
        if (xQueueReceive(uart_queue, (void * )&uart_event, portMAX_DELAY))
        {
            switch(uart_event.type)
            {
                case UART_DATA:
                    size_t buf_size;
                    uart_get_buffered_data_len(uart_num, &buf_size);
                    if (buf_size > UART_RX_BUF_SIZE) buf_size = UART_RX_BUF_SIZE;
                    uart_read_bytes(uart_num, data, buf_size, 100 / portTICK_PERIOD_MS);

                    if (xRingbufferSend(usb_sendbuf, data, buf_size, pdMS_TO_TICKS(10)) != pdTRUE)
                    {
                        ESP_LOGV(TAG, "Cannot write to ringbuffer (free %d of %d)!",
                                 xRingbufferGetCurFreeSize(usb_sendbuf),
                                 2048);
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                    // ESP_LOGI("UART_EVENT", "UART_DATA");
                    break;
                case UART_FIFO_OVF:
                    ESP_LOGW(TAG, "UART FIFO overflow");
                    uart_flush_input(uart_num);
                    xQueueReset(uart_queue);
                    break;
                case UART_BUFFER_FULL:
                    ESP_LOGW(TAG, "UART ring buffer full");
                    uart_flush_input(uart_num);
                    xQueueReset(uart_queue);
                    break;
                case UART_BREAK:
                    ESP_LOGI("UART_EVENT", "UART_BREAK");
                    break;
                case UART_PARITY_ERR:
                    ESP_LOGI("UART_EVENT", "UART_PARITY_ERR");
                    break;
                case UART_FRAME_ERR:
                    ESP_LOGI("UART_EVENT", "UART_FRAME_ERR");
                    break;
                default:
                    ESP_LOGI("UART_EVENT", "UNKNOWN");
                    break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelete(NULL);
}

void cdcSendTask(void *pvParameters)
{
    while (1)
    {
        size_t ringbuf_received;
        uint8_t *buf = (uint8_t *) xRingbufferReceiveUpTo(usb_sendbuf, &ringbuf_received, pdMS_TO_TICKS(100),
                       CFG_TUD_CDC_TX_BUFSIZE);

        if (buf)
        {
            uint8_t int_buf[CFG_TUD_CDC_TX_BUFSIZE];
            memcpy(int_buf, buf, ringbuf_received);
            vRingbufferReturnItem(usb_sendbuf, (void *) buf);

            for (int transferred = 0, to_send = ringbuf_received; transferred < ringbuf_received;)
            {
                //xSemaphoreGive(usb_tx_requested);
                const int wr_len = tud_cdc_write(int_buf + transferred, to_send);
                /* tinyusb might have been flushed the data. In case not flushed, we are flushing here.
                    2nd atttempt might return zero, meaning there is no data to transfer. So it is safe to call it again.
                */
                tud_cdc_write_flush();
                // if (usb_wait_for_tx(50) != ESP_OK) {
                //     //xSemaphoreTake(usb_tx_requested, 0);
                //     tud_cdc_write_clear(); /* host might be disconnected. drop the buffer */
                //     ESP_LOGV(TAG, "usb tx timeout");
                //     break;
                // }
                ESP_LOGD(TAG, "CDC ringbuffer -> CDC (%d bytes)", wr_len);
                transferred += wr_len;
                to_send -= wr_len;
            }
        } 
        else
        {
            ESP_LOGD(TAG, "usb_sender_task: nothing to send");
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
    }
    vTaskDelete(NULL);
}
