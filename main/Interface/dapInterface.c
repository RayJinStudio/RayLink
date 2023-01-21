#include <stdlib.h>

#include "DAP.h"
#include "DAP_config.h"

#include "dapInterface.h"

void dapSetUp()
{
    DAP_SETUP();
}

uint32_t dapProcessCommand(const uint8_t *request, uint8_t *response)
{
    return DAP_ProcessCommand(request, response);
}