#include "tusb.h"
#include "esp_log.h"

#include "cdcDevice.hpp"
#include "UartDrive.hpp"

#define TAG "cdcDevice"

uint8_t buf[CFG_TUD_CDC_RX_BUFSIZE];

void cdcCallback()
{
	return;
}

void tud_cdc_rx_cb(uint8_t itf)
{
    uint32_t rx_size = tud_cdc_n_read(itf, buf, CFG_TUD_CDC_RX_BUFSIZE);
    if (rx_size>0)
    {
        
        int transferred = uartSendData(buf, rx_size);
        if (transferred != rx_size)
        {
            ESP_LOGW(TAG, "uart_write_bytes transferred %d bytes only!", transferred);
        }
    }
    else
    {
        ESP_LOGW(TAG, "tud_cdc_rx_cb receive error");
    }
}

void tud_cdc_line_coding_cb(int itf, cdc_line_coding_t const *p_line_coding)
{
    static int last_bit_rate = -1;
    if (last_bit_rate != p_line_coding->bit_rate)
    {
        uartSetBaudrate(p_line_coding->bit_rate);
        last_bit_rate = p_line_coding->bit_rate;
    }
}
