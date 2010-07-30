#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <getopt.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include "pk2aux.h"



static const struct option LONG_OPTIONS[] = {
	{"device", required_argument, 0, 'd'},
	{"baud", required_argument, 0, 'b'},
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0}
};
static const char SHORT_OPTIONS[] = "d:b:h";



static int do_uart(const char *appname, pk2aux_handle handle, unsigned int poll_interval) {
	struct timeval tv;
	fd_set rfds, wfds, efds;
	unsigned char buffer[64];
	size_t length, sent;
	ssize_t rwrc;
	int selectrc;

	for (;;) {
		/* Try reading from the PICkit2's UART. */
		length = sizeof(buffer);
		if (pk2aux_receive_uart(handle, buffer, &length)) {
			perror(appname);
			return EXIT_FAILURE;
		}

		/* If we read some data, dump it to stdout. */
		sent = 0;
		while (sent < length) {
			/* Try to write the data. */
			do {
				rwrc = write(1, buffer + sent, length - sent);
			} while (rwrc < 0 && errno == EINTR);
			if (rwrc == 0 || (rwrc < 0 && errno == EAGAIN)) {
				/* The write would have blocked (although we set only stdin to nonblocking,
				 * if they are both the TTY, this will also set stdout to nonblocking).
				 * Use select with no timeout to wait until the write is possible. */
				FD_ZERO(&wfds);
				FD_SET(1, &wfds);
				selectrc = select(2, 0, &wfds, 0, 0);
				if (selectrc < 0 && errno != EINTR) {
					perror(appname);
					return EXIT_FAILURE;
				}
			} else if (rwrc < 0) {
				/* The write encountered an error. */
				perror(appname);
				return EXIT_FAILURE;
			} else {
				/* The write wrote rwrc bytes worth of data. */
				sent += rwrc;
			}
		}

		/* Wait up to 10ms to see some data from stdin - after that, go back and
		 * poll the PICkit2 again. */
		do {
			FD_ZERO(&rfds);
			FD_SET(0, &rfds);
			FD_ZERO(&efds);
			FD_SET(0, &efds);
			tv.tv_sec = poll_interval / 1000U;
			tv.tv_usec = (poll_interval % 1000U) * 1000U;
			selectrc = select(1, &rfds, 0, &efds, &tv);
		} while (selectrc < 0 && errno == EINTR);

		/* Check if there's data available for reading from stdin. */
		if (FD_ISSET(0, &rfds) || FD_ISSET(0, &efds)) {
			/* Read some data. */
			do {
				rwrc = read(0, buffer, sizeof(buffer));
			} while (rwrc < 0 && errno == EINTR);
			if (rwrc < 0 && errno != EAGAIN) {
				perror(appname);
				return EXIT_FAILURE;
			}
			if (rwrc == 0) {
				return EXIT_SUCCESS;
			}
			if (rwrc > 0) {
				/* Send the bytes to the PICkit2. */
				if (pk2aux_send_uart(handle, buffer, rwrc) < 0) {
					perror(appname);
					return EXIT_FAILURE;
				}
			}
		}
	}
}



static int parse_baud(const char *baud_string, unsigned int *baud, unsigned int *poll_interval) {
	char *ptr;
	unsigned long rc;

	if (!*baud_string)
		return -1;

	rc = strtoul(baud_string, &ptr, 10);
	if (*ptr)
		return -1;

	if (rc > UINT_MAX)
		return -1;

	*baud = (unsigned int) rc;
	*poll_interval = 1000U / (*baud / 10U / 128U) * 2U / 3U;
	return 0;
}



static void usage(const char *appname) {
	fprintf(stderr, "Usage: %s [options]\n"
			"Options:\n"
			" -d path, --device path      the path to the PICkit2, as printed by pk2ls\n"
			" -b speed, --baud speed      sets the baud rate of the serial port (REQUIRED, must be between 92 and 57600)\n"
			" -h, --help                  display this usage message\n",
			appname);
}



int main(int argc, char **argv) {
	int rc;
	char path[PATH_MAX + 1] = "";
	unsigned int baud = 0, poll_interval;
	int old_flags;
	pk2aux_device *device;
	pk2aux_handle handle;

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

			case 'b':
				if (parse_baud(optarg, &baud, &poll_interval) < 0 || baud < 92 || baud > 57600) {
					fprintf(stderr, "%s: baud rate '%s' is illegal\n", argv[0], optarg);
					return EXIT_FAILURE;
				}
				break;

			case 'h':
				usage(argv[0]);
				return EXIT_SUCCESS;

			default:
				return EXIT_FAILURE;
		}
	}

	if (optind != argc || !baud) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	/* Make standard input nonblocking so we can poll the PICkit2 regularly. */
	old_flags = fcntl(0, F_GETFL);
	if (old_flags < 0) {
		perror(argv[0]);
		return EXIT_FAILURE;
	}
	if (fcntl(0, F_SETFL, old_flags | O_NONBLOCK) < 0) {
		perror(argv[0]);
		return EXIT_FAILURE;
	}

	/* Scan for PICkit2s. */
	if (pk2aux_scan() < 0) {
		perror(argv[0]);
		return EXIT_FAILURE;
	}

	/* Find the requested device. */
	device = pk2aux_find_device(path);
	if (!device) {
		perror(argv[0]);
		return EXIT_FAILURE;
	}

	/* Open the device. */
	handle = pk2aux_open(device);
	if (!handle) {
		perror(argv[0]);
		return EXIT_FAILURE;
	}

	/* Enter UART mode. */
	if (pk2aux_start_uart(handle, baud) < 0) {
		perror(argv[0]);
		return EXIT_FAILURE;
	}

	/* Do UART stuff! */
	rc = do_uart(argv[0], handle, poll_interval);

	/* Restore old stdin flags. */
	fcntl(0, F_SETFL, old_flags);

	/* Finished. */
	pk2aux_stop_uart(handle);
	pk2aux_close(handle);
	return rc;
}

