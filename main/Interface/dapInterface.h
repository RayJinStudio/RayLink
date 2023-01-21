#ifndef DAPT_H
#define DAPT_H

#ifdef __cplusplus
extern "C"
{
#endif

void dapSetUp();
uint32_t dapProcessCommand(const uint8_t *request, uint8_t *response);

#ifdef __cplusplus
}
#endif

#endif