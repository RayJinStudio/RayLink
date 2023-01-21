#include <stdlib.h>

#include "tusb.h"
#include "tinyusb_types.h"

#include "esp_log.h"
#include "deviceDescriptor.hpp"

uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const *)&my_descriptor;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    return desc_configuration;
}

uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void) langid;

  uint8_t chr_count;

  if ( index == 0)
  {
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  }else
  {
    // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

    if ( !(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) ) return NULL;

    const char* str = string_desc_arr[index];

    // Cap at max char
    chr_count = (uint8_t) strlen(str);
    if ( chr_count > 31 ) chr_count = 31;

    // Convert ASCII string into UTF-16
    for(uint8_t i=0; i<chr_count; i++)
    {
      _desc_str[1+i] = str[i];
    }
  }

  // first byte is length (including header), second byte is string type
  _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8 ) | (2*chr_count + 2));

  return _desc_str;
}

uint8_t const * tud_descriptor_bos_cb(void)
{
  return desc_bos;
}


bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request)
{
  // nothing to with DATA & ACK stage
  if (stage != CONTROL_STAGE_SETUP) return true;

  switch (request->bmRequestType_bit.type)
  {
    case TUSB_REQ_TYPE_VENDOR:
      switch (request->bRequest)
      {
        case VENDOR_REQUEST_WEBUSB:
          // match vendor request in BOS descriptor
          // Get landing page url
          //return tud_control_xfer(rhport, request, (void*)(uintptr_t) &desc_url, desc_url.bLength);

        case VENDOR_REQUEST_MICROSOFT:
          if ( request->wIndex == 7 )
          {
            ESP_LOGI(TAG, "MS OS 2.0 request");
            // Get Microsoft OS 2.0 compatible descriptor
            uint16_t total_len;
            memcpy(&total_len, desc_ms_os_20+8, 2);

            return tud_control_xfer(rhport, request, (void*)(uintptr_t) desc_ms_os_20, total_len);
          }else
          {
            return false;
          }

        default: break;
      }
    break;

    case TUSB_REQ_TYPE_CLASS:
      if (request->bRequest == 0x22)
      {
        // Webserial simulate the CDC_REQUEST_SET_CONTROL_LINE_STATE (0x22) to connect and disconnect.
        // web_serial_connected = (request->wValue != 0);

        // // Always lit LED if connected
        // if ( web_serial_connected )
        // {
        //   //board_led_write(true);
        //   blink_interval_ms = BLINK_ALWAYS_ON;

        //   tud_vendor_write_str("\r\nWebUSB interface connected\r\n");
        //   //tud_vendor_flush();
        // }else
        // {
        //   blink_interval_ms = BLINK_MOUNTED;
        // }

        // // response with status OK
        // return tud_control_status(rhport, request);
      }
    break;

    default: break;
  }

  // stall unknown request
  return false;
}

extern "C" void init_serial_no(void)
{
    uint8_t m[MAC_BYTES] = {0};
    esp_err_t ret = esp_efuse_mac_get_default(m);

    if (ret != ESP_OK) {
        ESP_LOGD(TAG, "Cannot read MAC address and set the device serial number");
    }

    snprintf(serial_descriptor, sizeof(serial_descriptor),
             "%02X%02X%02X%02X%02X%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
}