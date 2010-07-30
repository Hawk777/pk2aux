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
#include "internal.h"



const char *pk2aux_error_string(int rc) {
	enum libusb_error ecode = rc;

	switch (ecode) {
		case LIBUSB_SUCCESS:
			return "No error";

		case LIBUSB_ERROR_IO:
			return "I/O error";

		case LIBUSB_ERROR_INVALID_PARAM:
			return "Invalid parameter";

		case LIBUSB_ERROR_ACCESS:
			return "Access denied";

		case LIBUSB_ERROR_NO_DEVICE:
			return "No such device";

		case LIBUSB_ERROR_NOT_FOUND:
			return "Entry not found";

		case LIBUSB_ERROR_BUSY:
			return "Resource busy";

		case LIBUSB_ERROR_TIMEOUT:
			return "Operation timed out";

		case LIBUSB_ERROR_OVERFLOW:
			return "Overflow";

		case LIBUSB_ERROR_PIPE:
			return "Pipe error";

		case LIBUSB_ERROR_INTERRUPTED:
			return "Interrupted";

		case LIBUSB_ERROR_NO_MEM:
			return "Insufficient memory";

		case LIBUSB_ERROR_NOT_SUPPORTED:
			return "Operation not supported";

		case LIBUSB_ERROR_OTHER:
			return "Unknown error";
	}

	return "Unknown error";
}

