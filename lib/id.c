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

