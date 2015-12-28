#include "debug.h"

#include "prj_usb_config.h"
#include "cdc.h"

#if true // NDEBUG

void debug_init(void) {}
void debug_service(void) {}

bool debug_can_send(void) {
	cdc_in_ready(USB_PIC_PORT);
}

void debug_send_data(char* data) {
	while(*data != '\0') {
		cdc_putc(USB_PIC_PORT, *data);	
		data++;
	}
}

void debug_send_cdata(const rom char* data) {
	while(*data != '\0') {
		cdc_putc(USB_PIC_PORT, *data);	
		data++;
	}
}

#endif // NDEBUG
