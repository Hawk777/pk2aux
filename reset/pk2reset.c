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
#include <libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



static const struct option LONG_OPTIONS[] = {
	{"device", required_argument, 0, 'd'},
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0}
};
static const char SHORT_OPTIONS[] = "d:h";



static int reset(const char *appname, const char *path) {
	int rc;
	pk2aux_device *device = 0;
	pk2aux_handle handle = 0;

	/* Initialize the library. */
	if ((rc = pk2aux_init()) < 0) {
		goto errout;
	}

	/* Find the device. */
	device = pk2aux_find_device(path);
	if (!device) {
		rc = LIBUSB_ERROR_NO_DEVICE;
		goto errout;
	}

	/* Open the device. */
	if ((rc = pk2aux_open(device, &handle)) < 0) {
		goto errout;
	}

	/* Reset the device. */
	pk2aux_reset(handle);
	handle = 0;

	rc = LIBUSB_SUCCESS;

out:
	if (handle) {
		pk2aux_close(handle);
		handle = 0;
	}
	pk2aux_exit();
	return rc == LIBUSB_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;

errout:
	fprintf(stderr, "%s: %s\n", appname, pk2aux_error_string(rc));
	goto out;
}



static void usage(const char *appname) {
	fprintf(stderr, "Usage: %s [options] new_unit_id\n"
			"Options:\n"
			" -d path, --device path      the path to the PICkit2, as printed by pk2ls\n"
			" -h, --help                  display this usage message\n"
			"\n"
			"Attempts to reset the PICkit2.\n",
		appname);
}



int main(int argc, char **argv) {
	int rc;
	const char *path = 0;

	while ((rc = getopt_long(argc, argv, SHORT_OPTIONS, LONG_OPTIONS, 0)) != -1) {
		switch (rc) {
			case 'd':
				path = optarg;
				break;

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

	return reset(argv[0], path);
}

