/*
 * Copyright 2008 Christopher Head
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
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

