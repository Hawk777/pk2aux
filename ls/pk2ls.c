#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include "pk2aux.h"



static const struct option LONG_OPTIONS[] = {
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0}
};
static const char SHORT_OPTIONS[] = "h";



static void usage(const char *appname) {
	fprintf(stderr,
			"Usage: %s [options]\n"
			"Options:\n"
			" -h, --help    display this usage message\n",
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

	/* Scan for devices. */
	if (pk2aux_scan() < 0) {
		perror(argv[0]);
		return 1;
	}

	/* Display a list of devices. */
	dlist = pk2aux_get_devices();
	for (i = 0; i < dlist.num_devices; i++) {
		printf("%s\t%s\n", dlist.devices[i].usb_path, dlist.devices[i].unit_id);
	}

	return 0;
}

