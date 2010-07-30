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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <getopt.h>
#include "pk2aux.h"



static const struct option LONG_OPTIONS[] = {
	{"device", required_argument, 0, 'd'},
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0}
};
static const char SHORT_OPTIONS[] = "d:h";



static int set_id(const char *appname, const char *path, const char *id) {
	pk2aux_device *device;
	pk2aux_handle handle;

	/* Find the device. */
	if (pk2aux_scan() < 0) {
		perror(appname);
		return EXIT_FAILURE;
	}
	device = pk2aux_find_device(path);
	if (!device) {
		perror(appname);
		return EXIT_FAILURE;
	}

	/* Open the device. */
	handle = pk2aux_open(device);
	if (!handle) {
		perror(appname);
		return EXIT_FAILURE;
	}

	/* Execute the write. */
	if (pk2aux_set_id(handle, id) < 0) {
		perror(appname);
		pk2aux_close(handle);
		return EXIT_FAILURE;
	}

	/* Close the handle. */
	pk2aux_close(handle);

	return EXIT_SUCCESS;
}



static void usage(const char *appname) {
	fprintf(stderr, "Usage: %s [options] new_unit_id\n"
			"Options:\n"
			" -d path, --device path      the path to the PICkit2, as printed by pk2ls\n"
			" -h, --help                  display this usage message\n",
			appname);
}



int main(int argc, char **argv) {
	int rc;
	char path[PATH_MAX + 1];

	path[0] = '\0';
	while ((rc = getopt_long(argc, argv, SHORT_OPTIONS, LONG_OPTIONS, 0)) != -1) {
		switch (rc) {
			case 'd':
				if (strlen(optarg) > PATH_MAX) {
					errno = ENAMETOOLONG;
					perror(argv[0]);
					return EXIT_FAILURE;
				}
				strcpy(path, optarg);
				break;

			case 'h':
				usage(argv[0]);
				return EXIT_SUCCESS;

			default:
				return EXIT_FAILURE;
		}
	}

	if (optind != argc - 1) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	return set_id(argv[0], path, argv[optind]);
}

