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
#include "pk2aux.h"
#include "internal.h"
#include "cmd.h"
#include <string.h>
#include <errno.h>
#include <usb.h>



int pk2aux_write_usb(usb_dev_handle *handle, const void *data, size_t length) {
	int rc;
	unsigned char buffer[64];

	if (length == 0)
		return 0;
	if (length > 64) {
		errno = EMSGSIZE;
		return -1;
	}

	memcpy(buffer, data, length);
	memset(buffer + length, END_OF_BUFFER, sizeof(buffer) - length);

	rc = usb_interrupt_write(handle, 0x01, (char *) buffer, sizeof(buffer), 1000);
	if (rc < 0) {
		errno = -rc;
		return -1;
	} else if (rc != sizeof(buffer)) {
		errno = EIO;
		return -1;
	}

	return 0;
}



int pk2aux_write(pk2aux_handle handle, const void *data, size_t length) {
	return pk2aux_write_usb(handle->usb_handle, data, length);
}



int pk2aux_read_usb(usb_dev_handle *handle, void *data) {
	int rc;

	rc = usb_interrupt_read(handle, 0x81, data, 64, 1000);
	if (rc < 0) {
		errno = -rc;
		return -1;
	} else if (rc != 64) {
		errno = EIO;
		return -1;
	}

	return 0;
}



int pk2aux_read(pk2aux_handle handle, void *data) {
	return pk2aux_read_usb(handle->usb_handle, data);
}

