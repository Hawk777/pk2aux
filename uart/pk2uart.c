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
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <libusb.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>



static const struct option LONG_OPTIONS[] = {
	{"device", required_argument, 0, 'd'},
	{"baud", required_argument, 0, 'b'},
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0}
};
static const char SHORT_OPTIONS[] = "d:b:h";



static int do_uart(const char *appname, pk2aux_handle handle, unsigned int poll_interval) {
	int rc;
	struct timeval tv;
	fd_set rfds, wfds, efds;
	unsigned char buffer[64];
	size_t length, sent;
	ssize_t rwrc;
	int selectrc;

	for (;;) {
		/* Try reading from the PICkit2's UART. */
		length = sizeof(buffer);
		if ((rc = pk2aux_receive_uart(handle, buffer, &length)) < 0) {
			fprintf(stderr, "%s: %s\n", appname, pk2aux_error_string(rc));
			return rc;
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
					return LIBUSB_ERROR_IO;
				}
			} else if (rwrc < 0) {
				/* The write encountered an error. */
				perror(appname);
				return LIBUSB_ERROR_IO;
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
				return LIBUSB_ERROR_IO;
			}
			if (rwrc == 0) {
				return LIBUSB_SUCCESS;
			}
			if (rwrc > 0) {
				/* Send the bytes to the PICkit2. */
				if ((rc = pk2aux_send_uart(handle, buffer, rwrc)) < 0) {
					fprintf(stderr, "%s: %s\n", appname, pk2aux_error_string(rc));
					return rc;
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
	*poll_interval = 1000U / *baud * 10U * 128U * 1U / 10U;
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
	const char *path = 0;
	unsigned int baud = 0, poll_interval = 0;
	int old_flags;
	pk2aux_device *device = 0;
	pk2aux_handle handle = 0;
	int in_uart_mode = 0;

	while ((rc = getopt_long(argc, argv, SHORT_OPTIONS, LONG_OPTIONS, 0)) != -1) {
		switch (rc) {
			case 'd':
				path = optarg;
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

	/* Initialize the library. */
	if ((rc = pk2aux_init()) < 0) {
		goto errout;
	}

	/* Find the requested device. */
	device = pk2aux_find_device(path);
	if (!device) {
		rc = LIBUSB_ERROR_NO_DEVICE;
		goto errout;
	}

	/* Open the device. */
	if ((rc = pk2aux_open(device, &handle)) < 0) {
		goto errout;
	}

	/* Enter UART mode. */
	if ((rc = pk2aux_start_uart(handle, baud)) < 0) {
		goto errout;
	}
	in_uart_mode = 1;

	/* Do UART stuff! */
	rc = do_uart(argv[0], handle, poll_interval);

out:
	if (in_uart_mode) {
		pk2aux_stop_uart(handle);
	}
	if (handle) {
		pk2aux_close(handle);
		handle = 0;
	}
	pk2aux_exit();
	fcntl(0, F_SETFL, old_flags);
	return rc == LIBUSB_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;

errout:
	fprintf(stderr, "%s: %s\n", argv[0], pk2aux_error_string(rc));
	goto out;
}

