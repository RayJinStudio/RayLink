#include "tusb.h"
#include "tusb_config.h"
#include "tinyusb_types.h"

#include "hal/usb_hal.h"
#include "soc/usb_periph.h"
#include "driver/gpio.h"
#include "driver/periph_ctrl.h"

#include "usbInit.h"

extern void init_serial_no(void);

static void configure_pins(usb_hal_context_t *usb)
{
    /* usb_periph_iopins currently configures USB_OTG as USB Device.
     * Introduce additional parameters in usb_hal_context_t when adding support
     * for USB Host.
     */
    for (const usb_iopin_dsc_t *iopin = usb_periph_iopins; iopin->pin != -1; ++iopin)
    {
        if ((usb->use_external_phy) || (iopin->ext_phy_only == 0))
        {
            gpio_pad_select_gpio(iopin->pin);
            if (iopin->is_output)
            {
                gpio_matrix_out(iopin->pin, iopin->func, false, false);
            } else
            {
                gpio_matrix_in(iopin->pin, iopin->func, false);
                gpio_pad_input_enable(iopin->pin);
            }
            gpio_pad_unhold(iopin->pin);
        }
    }
    if (!usb->use_external_phy)
    {
        gpio_set_drive_capability(USBPHY_DM_NUM, GPIO_DRIVE_CAP_3);
        gpio_set_drive_capability(USBPHY_DP_NUM, GPIO_DRIVE_CAP_3);
    }
}

void usbInit()
{
    init_serial_no();

    periph_module_reset(PERIPH_USB_MODULE);
    periph_module_enable(PERIPH_USB_MODULE);

    usb_hal_context_t hal = {
        .use_external_phy = false
    };
    usb_hal_init(&hal);
    configure_pins(&hal);

    tusb_init();
}