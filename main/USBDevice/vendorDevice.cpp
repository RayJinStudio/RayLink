#include "tusb.h"

#include "vendorDevice.hpp"
#include "dapInterface.h"

uint8_t WINUSB_Request[64] = {0};  // Request  Buffer
uint8_t WINUSB_Response[64] = {0}; // Response Buffer
uint8_t WINUSB_data = 0;
int WINUSB_len;

void vendorCallback()
{
	return;
}

void tud_vendor_rx_cb(uint8_t itf)
{
	uint32_t count = tud_vendor_read(WINUSB_Request, 64);
	dapProcessCommand(WINUSB_Request, WINUSB_Response);
	tud_vendor_write(WINUSB_Response, 63);
}