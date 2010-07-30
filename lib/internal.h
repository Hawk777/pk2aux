#if !defined INTERNAL_H
#define INTERNAL_H

#include <usb.h>
#include "pk2aux.h"

struct pk2aux_handle_impl {
	struct usb_dev_handle *usb_handle;
	unsigned int pgc_floating, pgd_floating, uart_enabled, uart_baud;
	unsigned char uart_buffer[63];
	size_t uart_buffer_used;
};

extern int pk2aux_write_usb(usb_dev_handle *handle, const void *data, size_t length);
extern int pk2aux_write(pk2aux_handle handle, const void *data, size_t length);
extern int pk2aux_read_usb(usb_dev_handle *handle, void *data);
extern int pk2aux_read(pk2aux_handle handle, void *data);

#endif

