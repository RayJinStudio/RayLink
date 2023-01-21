#ifndef UARTDRIVE_HPP
#define UARTDRIVE_HPP

int uartSendData(const uint8_t* data, int len);
bool uartSetBaudrate(const int baud);
void cdcSendTask(void *pvParameters);
void uartTask(void *pvParameters);
void UartInit(void);

#endif