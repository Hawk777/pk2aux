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
#include <string.h>
#include <errno.h>
#include <string.h>
#include <assert.h>



int pk2aux_set_id(pk2aux_handle handle, const char *id) {
	unsigned char buffer[19];

	/* Command. */
	buffer[0] = WR_INTERNAL_EE;
	/* Start address. */
	buffer[1] = 0xF0;
	/* Length. */
	buffer[2] = 16;

	/* Check whether the ID is to be removed. */
	if (id) {
		/* Check that the ID string is short enough. */
		if (strlen(id) > 15) {
			errno = ENOSPC;
			return -1;
		}
		/* A # character indicates that the ID string is valid. */
		buffer[3] = '#';
		memset(buffer + 4, 0, 15);
		strcpy((char *) buffer + 4, id);
	} else {
		memset(buffer + 3, 0xFF, 16);
	}

	if (pk2aux_write(handle, buffer, 19) < 0)
		return -1;

	return 0;
}

