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
#include "pk2aux.h"
#include "internal.h"
#include "cmd.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <usb.h>



static const uint16_t VID_MICROCHIP = 0x04D8;
static const uint16_t PID_PK2 = 0x0033;
static pk2aux_device *devices = 0;
static unsigned int num_devices = 0;



static int examine_device(struct usb_device *device) {
	struct usb_dev_handle *handle;
	unsigned char buffer[64];
	pk2aux_device *tmp;
	int saved_errno;

	/* Check vendor and product ID. */
	if (device->descriptor.idVendor != VID_MICROCHIP || device->descriptor.idProduct != PID_PK2) {
		return 0;
	}

	/* Open the device. We want to probe firmware version and see if it has a unit ID. */
	handle = usb_open(device);
	if (!handle) {
		return 0;
	}

	/* PICkit2s have 2 configurations; the first is HID and the second is non-HID.
	 * I suspect using the non-HID configuration may yield better results as it may
	 * make kernel drivers less likely to grab hold of the PICkit2. */
	if (usb_set_configuration(handle, 2) < 0) {
		usb_close(handle);
		return 0;
	}

	/* Claim the interface. */
	if (usb_claim_interface(handle, 0) < 0) {
		usb_close(handle);
		return 0;
	}

	/* First ask for firmware version. */
	buffer[0] = FIRMWARE_VERSION;
	if (pk2aux_write_usb(handle, buffer, 1) < 0) {
		saved_errno = errno;
		usb_release_interface(handle, 0);
		usb_close(handle);
		errno = saved_errno;
		return 0;
	}
	if (pk2aux_read_usb(handle, buffer) < 0) {
		saved_errno = errno;
		usb_release_interface(handle, 0);
		usb_close(handle);
		errno = saved_errno;
		return 0;
	}
	/* Assume that major version != 2 means an incompatible protocol, and maybe
	 * minor version < 30 means some commands we want aren't supported.
	 * The protocol datasheet is for version 2.30. */
	if (buffer[0] != 2 || buffer[1] < 30) {
		usb_release_interface(handle, 0);
		usb_close(handle);
		return 0;
	}

	/* Read the unit ID from the last 16 bytes of EEPROM. */
	buffer[0] = RD_INTERNAL_EE;
	buffer[1] = 0xF0;
	buffer[2] = 16;
	if (pk2aux_write_usb(handle, buffer, 3) < 0) {
		saved_errno = errno;
		usb_release_interface(handle, 0);
		usb_close(handle);
		errno = saved_errno;
		return 0;
	}
	if (pk2aux_read_usb(handle, buffer) < 0) {
		saved_errno = errno;
		usb_release_interface(handle, 0);
		usb_close(handle);
		errno = saved_errno;
		return 0;
	}

	/* Release and close the device. */
	usb_release_interface(handle, 0);
	usb_close(handle);

	/* Allocate a new device structure to hold the new device's data. */
	if (devices) {
		tmp = realloc(devices, (num_devices + 1) * sizeof(pk2aux_device));
	} else {
		tmp = malloc((num_devices + 1) * sizeof(pk2aux_device));
	}
	if (!tmp) {
		saved_errno = errno;
		if (devices) {
			free(devices);
			devices = 0;
			num_devices = 0;
		}
		usb_release_interface(handle, 0);
		usb_close(handle);
		errno = saved_errno;
		return -1;
	}
	devices = tmp;

	/* Fill out the device structure. The unit ID always starts with a # character
	 * if it's been programmed by the standard application. */
	memset(devices[num_devices].unit_id, 0, 16);
	if (buffer[0] == '#') {
		memcpy(devices[num_devices].unit_id, buffer + 1, 15);
	}
	strcpy(devices[num_devices].usb_path, device->bus->dirname);
	strcat(devices[num_devices].usb_path, "/");
	strcat(devices[num_devices].usb_path, device->filename);
	devices[num_devices].private_data = device;
	num_devices++;

	return 0;
}



int pk2aux_scan(void) {
	struct usb_bus *bus;
	struct usb_device *device;

	/* Initialize libusb. */
	usb_init();
	usb_find_busses();
	usb_find_devices();

	/* Free any existing block of device structures. */
	if (devices) {
		free(devices);
		devices = 0;
		num_devices = 0;
	}

	/* Walk the busses looking for PICkit2 devices. */
	for (bus = usb_get_busses(); bus; bus = bus->next) {
		for (device = bus->devices; device; device = device->next) {
			if (examine_device(device) < 0) {
				return -1;
			}
		}
	}

	return 0;
}



pk2aux_device *pk2aux_find_device(const char *path) {
	unsigned int i;

	/* Check that we have initialized and found a list of devices. */
	if (!devices) {
		errno = ENODEV;
		return 0;
	}

	/* If the path is empty, then only succeed if there is exactly one device. */
	if (!path[0]) {
		if (num_devices == 1) {
			return &devices[0];
		} else {
			errno = ENODEV;
			return 0;
		}
	}

	/* Scan for the requested device. */
	for (i = 0; i < num_devices; i++) {
		if (strcmp(path, devices[i].usb_path) == 0) {
			return &devices[i];
		}
	}

	errno = ENODEV;
	return 0;
}



pk2aux_device_list pk2aux_get_devices(void) {
	pk2aux_device_list dlist;

	dlist.num_devices = num_devices;
	dlist.devices = devices;
	return dlist;
}



pk2aux_handle pk2aux_open(pk2aux_device *device) {
	pk2aux_handle handle;
	int saved_errno, rc;
	unsigned char buffer[64];

	/* Allocate space for the private data structure. */
	handle = malloc(sizeof(*handle));
	if (!handle) {
		return 0;
	}

	/* Open the PICkit2. */
	handle->usb_handle = usb_open((struct usb_device *) device->private_data);
	if (!handle->usb_handle) {
		saved_errno = errno;
		free(handle);
		errno = saved_errno;
		return 0;
	}

	/* Set it to the non-HID configuration. */
	rc = usb_set_configuration(handle->usb_handle, 2);
	if (rc < 0) {
		usb_close(handle->usb_handle);
		free(handle);
		errno = -rc;
		return 0;
	}

	/* Claim the interface containing the two endpoints. */
	rc = usb_claim_interface(handle->usb_handle, 0);
	if (rc < 0) {
		usb_close(handle->usb_handle);
		free(handle);
		errno = -rc;
		return 0;
	}

	/* The firmware doesn't contain any commands directly intended to probe
	 * for the current state of the pins (i.e. whether VDD/VPP/PGC/PGD/AUX
	 * are grounded, high, or floating). Unfortunately, the PGC and PGD pins
	 * are tied together in such a way that the only SET command that affects
	 * either of those pins sets the states of both pins simultaneously. This
	 * means it is impossible to write a function to set the state of one pin
	 * without changing the state of the other pin, in the general case. Since
	 * this would be highly desirable functionality, the code following is a
	 * horrible horrible hack which accomplishes just that: it uses the PEEK SFR
	 * function to peek at the TRISA register in order to determine whether each
	 * of PGC and PGD are currently inputs or outputs. Once this determination is
	 * made, further queries to determine actual voltage levels can be accomplished
	 * by means of the regular ICSP_STATES_BUFFER command. */
	buffer[0] = EXECUTE_SCRIPT;
	buffer[1] = 2;
	buffer[2] = PEEK_SFR;
	buffer[3] = 0x92; /* TRISA */
	buffer[4] = UPLOAD_DATA;
	if (pk2aux_write(handle, buffer, 5) < 0) {
		saved_errno = errno;
		usb_release_interface(handle->usb_handle, 0);
		usb_close(handle->usb_handle);
		free(handle);
		errno = saved_errno;
		return 0;
	}

	if (pk2aux_read(handle, buffer) < 0) {
		saved_errno = errno;
		usb_release_interface(handle->usb_handle, 0);
		usb_close(handle->usb_handle);
		free(handle);
		errno = saved_errno;
		return 0;
	}

	assert(buffer[0] == 1);
	handle->pgc_floating = (buffer[buffer[0]] & 0x08) ? 1 : 0; /* PGC is RA3 */
	handle->pgd_floating = (buffer[buffer[0]] & 0x04) ? 1 : 0; /* PGD is RA2 */

	handle->uart_enabled = 0;

	return handle;
}



void pk2aux_reset(pk2aux_handle handle) {
	unsigned char buffer[1];

	if (handle->uart_enabled)
		pk2aux_stop_uart(handle);

	buffer[0] = RESET;
	pk2aux_write(handle, buffer, 1);
	usb_reset(handle->usb_handle);
	usb_close(handle->usb_handle);
	free(handle);
}



void pk2aux_close(pk2aux_handle handle) {
	if (handle->uart_enabled)
		pk2aux_stop_uart(handle);

	usb_release_interface(handle->usb_handle, 0);
	usb_close(handle->usb_handle);
	free(handle);
}



int pk2aux_get_version(pk2aux_handle handle, unsigned int *major, unsigned int *minor, unsigned int *micro) {
	unsigned char buffer[64];

	buffer[0] = FIRMWARE_VERSION;
	if (pk2aux_write(handle, buffer, 1) < 0)
		return -1;

	if (pk2aux_read(handle, buffer) < 0)
		return -1;

	*major = buffer[0];
	*minor = buffer[1];
	*micro = buffer[2];
	return 0;
}

