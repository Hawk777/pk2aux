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
#include "pk2aux.h"
#include "internal.h"
#include "cmd.h"
#include <errno.h>
#include <math.h>
#include <assert.h>
#include <string.h>



int pk2aux_start_uart(pk2aux_handle handle, unsigned int baud) {
	unsigned int brg;
	unsigned char buffer[3];

	/* 92 is the smallest baud that gives a positive BRG
	 * 57600 is the largest baud the spec sheet specifies as legal */
	if (baud < 92 || baud > 57600) {
		errno = EDOM;
		return -1;
	}

	brg = (unsigned int) ((65536.0 - (((1.0 / baud) - 3.0e-6) / 1.67e-7)) + 0.5);
	assert(brg < 65536);

	if (handle->uart_enabled)
		if (pk2aux_stop_uart(handle) < 0)
			return -1;
	
	buffer[0] = ENTER_UART_MODE;
	buffer[1] = (unsigned char) (brg & 0xFF);
	buffer[2] = (unsigned char) (brg >> 8);
	if (pk2aux_write(handle, buffer, 3) < 0)
		return -1;

	handle->uart_enabled = 1;
	handle->uart_baud = baud;
	handle->uart_buffer_used = 0;

	return 0;
}



int pk2aux_stop_uart(pk2aux_handle handle) {
	unsigned char buffer[2];

	if (!handle->uart_enabled)
		return 0;

	buffer[0] = EXIT_UART_MODE;
	buffer[1] = CLR_UPLOAD_BUFFER;
	if (pk2aux_write(handle, buffer, 2) < 0)
		return -1;

	handle->uart_enabled = 0;
	return 0;
}



int pk2aux_receive_uart(pk2aux_handle handle, void *data, size_t *length) {
	unsigned char buffer[64];

	/* If we're not in UART mode, we have no data to present. */
	if (!handle->uart_enabled) {
		*length = 0;
		return 0;
	}

	/* If we have any data buffered, present it first (the buffer must be 100% empty
	 * before we can request another block from the device). */
	if (handle->uart_buffer_used) {
		if (handle->uart_buffer_used < *length)
			*length = handle->uart_buffer_used;
		memcpy(data, handle->uart_buffer, *length);
		memmove(data, data + *length, handle->uart_buffer_used - *length);
		handle->uart_buffer_used -= *length;
		return 0;
	}

	/* Receive some data. */
	buffer[0] = UPLOAD_DATA;
	if (pk2aux_write(handle, buffer, 1) < 0)
		return -1;

	if (pk2aux_read(handle, buffer) < 0)
		return -1;

	/* Copy what we can into the application's buffer. */
	if (*length > buffer[0])
		*length = buffer[0];
	memcpy(data, buffer + 1, *length);

	/* Copy the rest into the buffer in the handle. */
	handle->uart_buffer_used = buffer[0] - *length;
	memcpy(handle->uart_buffer, buffer + 1 + *length, handle->uart_buffer_used);

	return 0;
}



int pk2aux_send_uart(pk2aux_handle handle, const void *data, size_t length) {
	unsigned char buffer[64];
	size_t to_send;
	unsigned int to_sleep_total, to_sleep_this;

	/* If we're not in UART mode, fail. */
	if (!handle->uart_enabled) {
		errno = -EIO;
		return -1;
	}

	/* Keep going as long as there's data left. */
	while (length) {
		/* Try to send up to 62 bytes (that's all that fits in a single USB transaction). */
		if (length > 62)
			to_send = 62;
		else
			to_send = length;

		buffer[0] = DOWNLOAD_DATA;
		buffer[1] = (unsigned char) to_send;
		memcpy(buffer + 2, data, to_send);
		if (pk2aux_write(handle, buffer, to_send + 2) < 0)
			return -1;

		/* Sleep for the appropriate amount of time to allow the data to drain (we don't want
		 * to overflow the download buffer, and there's no way to query how much data is in it). */
		to_sleep_total = 1000U * to_send * 11U / handle->uart_baud;
		while (to_sleep_total) {
			if (to_sleep_total > 1000U)
				to_sleep_this = 1000000U;
			else
				to_sleep_this = to_sleep_total * 1000U;
			usleep(to_sleep_this);
			to_sleep_total -= to_sleep_this / 1000U;
		}

		/* Consume the data. */
		data = ((char *) data) + to_send;
		length -= to_send;
	}

	return 0;
}

