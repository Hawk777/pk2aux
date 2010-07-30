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
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "pk2aux.h"



#define VDD_OPT 1
#define VPP_OPT 2
#define PGC_OPT 3
#define PGD_OPT 4
#define AUX_OPT 5
static const struct option LONG_OPTIONS[] = {
	{"device", required_argument, 0, 'd'},
	{"vdd", required_argument, 0, VDD_OPT},
	{"vpp", required_argument, 0, VPP_OPT},
	{"pgc", required_argument, 0, PGC_OPT},
	{"pgd", required_argument, 0, PGD_OPT},
	{"aux", required_argument, 0, AUX_OPT},
	{"query", no_argument, 0, 'q'},
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0}
};
static const char SHORT_OPTIONS[] = "hq";



static int parse_mode(const char *mode_string, enum PIN_MODE *mode) {
	if (strcmp(mode_string, "grounded") == 0) {
		*mode = PIN_MODE_GROUNDED;
		return 0;
	} else if (strcmp(mode_string, "floating") == 0) {
		*mode = PIN_MODE_FLOATING;
		return 0;
	} else if (strcmp(mode_string, "high") == 0) {
		*mode = PIN_MODE_HIGH;
		return 0;
	}
	return -1;
}



static int parse_level(const char *level_string, double *level, double min_level, double max_level) {
	char *endptr;

	errno = 0;
	*level = strtod(level_string, &endptr);
	if (errno != 0)
		return -1;
	if (*endptr != '\0')
		return -1;
	if (*level < min_level || *level > max_level)
		return -1;
	return 0;
}



static int do_query(pk2aux_handle handle) {
	double voltage;
	unsigned int level;

	if (pk2aux_get_vdd_level(handle, &voltage) < 0)
		return -1;
	printf("VDD: %.2f\n", voltage);

	if (pk2aux_get_vpp_level(handle, &voltage) < 0)
		return -1;
	printf("VPP: %.2f\n", voltage);

	if (pk2aux_get_pgc(handle, &level) < 0)
		return -1;
	printf("PGC: %d\n", level);

	if (pk2aux_get_pgd(handle, &level) < 0)
		return -1;
	printf("PGD: %d\n", level);

	if (pk2aux_get_aux(handle, &level) < 0)
		return -1;
	printf("AUX: %d\n", level);

	return 0;
}



static void usage(const char *appname) {
	fprintf(stderr,
			"Usage: %s options\n"
			"Options:\n"
			" -h, --help               displays this usage message\n"
			" -d path, --device path   the path to the PICkit2, as printed by pk2ls\n"
			" --vdd level              where 0.0 <= level <= 5.0\n"
			" --vdd mode               where mode is one of `grounded', `floating', `high'\n"
			" --vpp level              where 0.0 <= level <= 13.7\n"
			" --vpp mode               where mode is one of `grounded', `floating', `high'\n"
			" --pgc mode               where mode is one of `grounded', `floating', `high'\n"
			" --pgd mode               where mode is one of `grounded', `floating', `high'\n"
			" --aux mode               where mode is one of `grounded', `floating', `high'\n"
			" --query                  show the levels of VDD/VPP, states of PGC/PGD/AUX\n",
			appname);
}



int main(int argc, char **argv) {
	int rc;
	char path[PATH_MAX + 1];
	int vdd_set_mode = 0, vpp_set_mode = 0, pgc_set_mode = 0, pgd_set_mode = 0, aux_set_mode = 0;
	enum PIN_MODE vdd_mode = PIN_MODE_GROUNDED, vpp_mode = PIN_MODE_GROUNDED;
	enum PIN_MODE pgc_mode = PIN_MODE_GROUNDED, pgd_mode = PIN_MODE_GROUNDED;
	enum PIN_MODE aux_mode = PIN_MODE_GROUNDED;
	double vdd_level = -1.0, vpp_level = -1.0;
	unsigned int query = 0;
	pk2aux_device *device = 0;
	pk2aux_handle handle = 0;

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

			case VDD_OPT:
				if (parse_mode(optarg, &vdd_mode) == 0) {
					vdd_set_mode = 1;
					break;
				}
				if (parse_level(optarg, &vdd_level, 0.0, 5.0) == 0)
					break;
				fprintf(stderr, "%s: unrecognized VDD mode/level\n", argv[0]);
				return EXIT_FAILURE;

			case VPP_OPT:
				if (parse_mode(optarg, &vpp_mode) == 0) {
					vpp_set_mode = 1;
					break;
				}
				if (parse_level(optarg, &vpp_level, 0.0, 13.7) == 0)
					break;
				fprintf(stderr, "%s: unrecognized VPP mode/level\n", argv[0]);
				return EXIT_FAILURE;

			case PGC_OPT:
				if (parse_mode(optarg, &pgc_mode) == 0) {
					pgc_set_mode = 1;
					break;
				}
				fprintf(stderr, "%s: unrecognized PGC mode\n", argv[0]);
				return EXIT_FAILURE;

			case PGD_OPT:
				if (parse_mode(optarg, &pgd_mode) == 0) {
					pgd_set_mode = 1;
					break;
				}
				fprintf(stderr, "%s: unrecognized PGD mode\n", argv[0]);
				return EXIT_FAILURE;

			case AUX_OPT:
				if (parse_mode(optarg, &aux_mode) == 0) {
					aux_set_mode = 1;
					break;
				}
				fprintf(stderr, "%s: unrecognized AUX mode\n", argv[0]);
				return EXIT_FAILURE;

			case 'h':
				usage(argv[0]);
				return EXIT_SUCCESS;

			case 'q':
				query = 1;
				break;

			case 0:
				break;

			default:
				return EXIT_FAILURE;
		}
	}

	if (optind != argc) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	/* Find the device. */
	if (pk2aux_scan() < 0)
		goto errout;
	device = pk2aux_find_device(path);
	if (!device)
		goto errout;

	/* Open the device. */
	handle = pk2aux_open(device);
	if (!handle)
		goto errout;

	/* If VDD/VPP are having both levels and modes set, it the order in which we do these
	 * depends on the mode being set. If the mode is being set to HIGH, we want to set
	 * the level first so that the target circuit doesn't see the old level for a moment.
	 * If the mode is being set to GROUNDED or FLOATING, we want to set the mode first so
	 * that the target circuit doesn't see the *new* level for a moment. */
	if (vdd_set_mode && vdd_mode != PIN_MODE_HIGH && vdd_level > -0.5)
		if (pk2aux_set_vdd_mode(handle, vdd_mode) < 0)
			goto errout;
	if (vpp_set_mode && vpp_mode != PIN_MODE_HIGH && vpp_level > -0.5)
		if (pk2aux_set_vpp_mode(handle, vpp_mode) < 0)
			goto errout;

	/* Next set the levels. */
	if (vdd_level > -0.5)
		if (pk2aux_set_vdd_level(handle, vdd_level) < 0)
			goto errout;
	if (vpp_level > -0.5)
		if (pk2aux_set_vpp_level(handle, vpp_level) < 0)
			goto errout;

	/* If we're setting the VPP mode to high and we also changed its voltage, sleep for
	 * 100ms here to allow the voltage multiplier to stabilize. */
	usleep(100000);

	/* Set the modes of all the pins whose modes were requested to be changed. */
	if (vdd_set_mode)
		if (pk2aux_set_vdd_mode(handle, vdd_mode) < 0)
			goto errout;
	if (vpp_set_mode)
		if (pk2aux_set_vpp_mode(handle, vpp_mode) < 0)
			goto errout;
	if (pgc_set_mode)
		if (pk2aux_set_pgc(handle, pgc_mode) < 0)
			goto errout;
	if (pgd_set_mode)
		if (pk2aux_set_pgd(handle, pgd_mode) < 0)
			goto errout;
	if (aux_set_mode)
		if (pk2aux_set_aux(handle, aux_mode) < 0)
			goto errout;

	/* If we were given the query option, do the query and display the results. */
	if (query)
		if (do_query(handle) < 0)
			goto errout;

	rc = EXIT_SUCCESS;

out:
	if (handle)
		pk2aux_close(handle);
	return rc;

errout:
	perror(argv[0]);
	rc = EXIT_FAILURE;
	goto out;
}

