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
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>



static const struct option LONG_OPTIONS[] = {
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0}
};
static const char SHORT_OPTIONS[] = "h";



static void usage(const char *appname) {
	fprintf(stderr,
			"Usage: %s [options]\n"
			"Options:\n"
			" -h, --help    display this usage message\n"
			"\n"
			"Displays a list of all PICkit2 devices attached to the system, along with the\n"
			"bus number and device address of each.\n",
		appname);
}



int main(int argc, char **argv) {
	pk2aux_device_list dlist;
	unsigned int i;
	int rc;

	/* Parse arguments. */
	while ((rc = getopt_long(argc, argv, SHORT_OPTIONS, LONG_OPTIONS, 0)) != -1) {
		switch (rc) {
			case 'h':
				usage(argv[0]);
				return EXIT_SUCCESS;
				
			default:
				return EXIT_FAILURE;
		}
	}
	if (optind != argc) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	/* Initialize the library. */
	if ((rc = pk2aux_init()) < 0) {
		fprintf(stderr, "%s: %s\n", argv[0], pk2aux_error_string(rc));
		return 1;
	}

	/* Display a list of devices. */
	dlist = pk2aux_get_devices();
	for (i = 0; i < dlist.num_devices; i++) {
		printf("%d:%d\t%s\n", dlist.devices[i].bus_number, dlist.devices[i].device_address, dlist.devices[i].unit_id);
	}

	/* Deinitialize the library. */
	pk2aux_exit();

	return 0;
}

