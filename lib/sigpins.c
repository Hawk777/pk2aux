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
#include <assert.h>



static int query_pg(pk2aux_handle handle, unsigned char *result) {
	int rc;
	unsigned char buffer[64];
	
	buffer[0] = EXECUTE_SCRIPT;
	buffer[1] = 1;
	buffer[2] = ICSP_STATES_BUFFER;
	buffer[3] = UPLOAD_DATA;
	if ((rc = pk2aux_write(handle, buffer, 4)) < 0) {
		return rc;
	}

	if ((rc = pk2aux_read(handle, buffer)) < 0) {
		return rc;
	}

	assert(buffer[0] == 1);
	*result = buffer[1];
	return 0;
}



static int get_pg_modes(pk2aux_handle handle, enum PIN_MODE *pgc, enum PIN_MODE *pgd) {
	int rc;
	unsigned char levels;

	if (handle->pgc_floating && handle->pgd_floating) {
		/* No need to waste USB bandwidth in this case. */
		*pgc = PIN_MODE_FLOATING;
		*pgd = PIN_MODE_FLOATING;
		return 0;
	}

	if ((rc = query_pg(handle, &levels)) < 0) {
		return rc;
	}

	if (pgc) {
		*pgc = handle->pgc_floating ? PIN_MODE_FLOATING : ((levels & 0x01) ? PIN_MODE_HIGH : PIN_MODE_GROUNDED);
	}

	if (pgd) {
		*pgd = handle->pgd_floating ? PIN_MODE_FLOATING : ((levels & 0x02) ? PIN_MODE_HIGH : PIN_MODE_GROUNDED);
	}

	return 0;
}



static int get_pg_levels(pk2aux_handle handle, unsigned int *pgc, unsigned int *pgd) {
	int rc;
	unsigned char levels;

	if ((rc = query_pg(handle, &levels)) < 0) {
		return rc;
	}

	if (pgc) {
		*pgc = (levels & 0x01) ? 1 : 0;
	}

	if (pgd) {
		*pgd = (levels & 0x02) ? 1 : 0;
	}

	return 0;
}



static int set_pg_modes(pk2aux_handle handle, enum PIN_MODE pgc, enum PIN_MODE pgd) {
	int rc;
	unsigned char pgc_bits, pgd_bits;
	unsigned char buffer[4];

	pgc_bits = pgc == PIN_MODE_FLOATING ? 0x01 : pgc == PIN_MODE_HIGH ? 0x04 : 0x00;
	pgd_bits = pgd == PIN_MODE_FLOATING ? 0x02 : pgd == PIN_MODE_HIGH ? 0x08 : 0x00;

	buffer[0] = EXECUTE_SCRIPT;
	buffer[1] = 2;
	buffer[2] = SET_ICSP_PINS;
	buffer[3] = pgc_bits | pgd_bits;
	if ((rc = pk2aux_write(handle, buffer, 4)) < 0) {
		return rc;
	}

	handle->pgc_floating = pgc == PIN_MODE_FLOATING;
	handle->pgd_floating = pgd == PIN_MODE_FLOATING;
	return 0;
}



int pk2aux_set_pgc(pk2aux_handle handle, enum PIN_MODE mode) {
	int rc;
	enum PIN_MODE pgd_mode;

	if ((rc = get_pg_modes(handle, 0, &pgd_mode)) < 0) {
		return rc;
	}

	return set_pg_modes(handle, mode, pgd_mode);
}



int pk2aux_set_pgd(pk2aux_handle handle, enum PIN_MODE mode) {
	int rc;
	enum PIN_MODE pgc_mode;

	if ((rc = get_pg_modes(handle, &pgc_mode, 0)) < 0) {
		return rc;
	}

	return set_pg_modes(handle, pgc_mode, mode);
}



int pk2aux_set_aux(pk2aux_handle handle, enum PIN_MODE mode) {
	unsigned char buffer[4];

	buffer[0] = EXECUTE_SCRIPT;
	buffer[1] = 2;
	buffer[2] = SET_AUX;
	buffer[3] = mode == PIN_MODE_FLOATING ? 0x01 : mode == PIN_MODE_HIGH ? 0x02 : 0x00;

	return pk2aux_write(handle, buffer, 4);
}



int pk2aux_get_pgc(pk2aux_handle handle, unsigned int *level) {
	return get_pg_levels(handle, level, 0);
}



int pk2aux_get_pgd(pk2aux_handle handle, unsigned int *level) {
	return get_pg_levels(handle, 0, level);
}



int pk2aux_get_aux(pk2aux_handle handle, unsigned int *level) {
	int rc;
	unsigned char buffer[64];

	buffer[0] = EXECUTE_SCRIPT;
	buffer[1] = 1;
	buffer[2] = AUX_STATE_BUFFER;
	buffer[3] = UPLOAD_DATA;
	if ((rc = pk2aux_write(handle, buffer, 4)) < 0) {
		return rc;
	}

	if ((rc = pk2aux_read(handle, buffer)) < 0) {
		return rc;
	}

	assert(buffer[0] == 1);
	*level = buffer[1] & 0x01 ? 1 : 0;

	return 0;
}

