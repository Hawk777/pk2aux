/*
 * Copyright 2008 Christopher Head
 *
 * This file is part of PK2Aux.
 *
 * PK2Aux is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PK2Aux is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PK2Aux.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "cmd.h"
#include "internal.h"
#include <string.h>



int pk2aux_write_usb(libusb_device_handle *handle, const void *data, size_t length) {
	unsigned char buffer[64];
	int transferred;

	if (length == 0) {
		return 0;
	}

	if (length > 64) {
		return LIBUSB_ERROR_OVERFLOW;
	}

	memcpy(buffer, data, length);
	memset(buffer + length, END_OF_BUFFER, sizeof(buffer) - length);

	return libusb_interrupt_transfer(handle, 0x01, buffer, sizeof(buffer), &transferred, 1000);
}



int pk2aux_write(pk2aux_handle handle, const void *data, size_t length) {
	return pk2aux_write_usb(handle->usb_handle, data, length);
}



int pk2aux_read_usb(libusb_device_handle *handle, void *data) {
	int transferred;

	return libusb_interrupt_transfer(handle, 0x81, data, 64, &transferred, 1000);
}



int pk2aux_read(pk2aux_handle handle, void *data) {
	return pk2aux_read_usb(handle->usb_handle, data);
}

