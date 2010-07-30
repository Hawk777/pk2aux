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
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



static const uint16_t VID_MICROCHIP = 0x04D8;
static const uint16_t PID_PK2 = 0x0033;
static libusb_context *usb_context = 0;
static pk2aux_device *devices = 0;
static unsigned int num_devices = 0;



static int examine_device(libusb_device *device) {
	libusb_device_handle *handle = 0;
	struct libusb_device_descriptor ddev;
	unsigned char buffer[64];
	pk2aux_device *tmp = 0;
	int original_config, tmp_config;

	/* Get device descriptor. */
	if (libusb_get_device_descriptor(device, &ddev) < 0) {
		return 0;
	}

	/* Check vendor and product ID. */
	if (ddev.idVendor != VID_MICROCHIP || ddev.idProduct != PID_PK2) {
		return 0;
	}

	/* Open the device. We want to probe firmware version and see if it has a unit ID. */
	if (libusb_open(device, &handle) < 0) {
		return 0;
	}

	/* Get the configuration index the device was originally in. */
	if (libusb_get_configuration(handle, &original_config) < 0) {
		libusb_close(handle);
		return 0;
	}
	if (!original_config) {
		original_config = -1;
	}

	/* PICkit2s have 2 configurations; the first is HID and the second is non-HID.
	 * I suspect using the non-HID configuration may yield better results as it may
	 * make kernel drivers less likely to grab hold of the PICkit2. */
	if (original_config != 2) {
		if (libusb_set_configuration(handle, 2) < 0) {
			libusb_close(handle);
			return 0;
		}
	}

	/* Claim the interface. */
	if (libusb_claim_interface(handle, 0) < 0) {
		if (original_config != 2) {
			libusb_set_configuration(handle, original_config);
		}
		libusb_close(handle);
		return 0;
	}

	/* Check that the configuration index hasn't changed since. */
	if (libusb_get_configuration(handle, &tmp_config) < 0) {
		libusb_release_interface(handle, 0);
		libusb_close(handle);
		return 0;
	}
	if (tmp_config != 2) {
		libusb_release_interface(handle, 0);
		libusb_close(handle);
		return 0;
	}

	/* First ask for firmware version. */
	buffer[0] = FIRMWARE_VERSION;
	if (pk2aux_write_usb(handle, buffer, 1) < 0) {
		libusb_release_interface(handle, 0);
		if (original_config != 2) {
			libusb_set_configuration(handle, original_config);
		}
		libusb_close(handle);
		return 0;
	}
	if (pk2aux_read_usb(handle, buffer) < 0) {
		libusb_release_interface(handle, 0);
		if (original_config != 2) {
			libusb_set_configuration(handle, original_config);
		}
		libusb_close(handle);
		return 0;
	}
	/* Assume that major version != 2 means an incompatible protocol, and maybe
	 * minor version < 30 means some commands we want aren't supported.
	 * The protocol datasheet is for version 2.30. */
	if (buffer[0] != 2 || buffer[1] < 30) {
		libusb_release_interface(handle, 0);
		if (original_config != 2) {
			libusb_set_configuration(handle, original_config);
		}
		libusb_close(handle);
		return 0;
	}

	/* Read the unit ID from the last 16 bytes of EEPROM. */
	buffer[0] = RD_INTERNAL_EE;
	buffer[1] = 0xF0;
	buffer[2] = 16;
	if (pk2aux_write_usb(handle, buffer, 3) < 0) {
		libusb_release_interface(handle, 0);
		if (original_config != 2) {
			libusb_set_configuration(handle, original_config);
		}
		libusb_close(handle);
		return 0;
	}
	if (pk2aux_read_usb(handle, buffer) < 0) {
		libusb_release_interface(handle, 0);
		if (original_config != 2) {
			libusb_set_configuration(handle, original_config);
		}
		libusb_close(handle);
		return 0;
	}

	/* Release and close the device. */
	libusb_release_interface(handle, 0);
	if (original_config != 2) {
		libusb_set_configuration(handle, original_config);
	}
	libusb_close(handle);
	handle = 0;

	/* Allocate a new device structure to hold the new device's data. */
	if (devices) {
		tmp = realloc(devices, (num_devices + 1) * sizeof(pk2aux_device));
	} else {
		tmp = malloc((num_devices + 1) * sizeof(pk2aux_device));
	}
	if (!tmp) {
		if (devices) {
			free(devices);
			devices = 0;
			num_devices = 0;
		}
		return LIBUSB_ERROR_NO_MEM;
	}
	devices = tmp;

	/* Fill out the device structure. The unit ID always starts with a # character
	 * if it's been programmed by the standard application. */
	memset(devices[num_devices].unit_id, 0, 16);
	if (buffer[0] == '#') {
		memcpy(devices[num_devices].unit_id, buffer + 1, 15);
	}
	devices[num_devices].bus_number = libusb_get_bus_number(device);
	devices[num_devices].device_address = libusb_get_device_address(device);
	devices[num_devices].private_data = device;
	num_devices++;

	/* Keep the libusb device structure in memory. */
	libusb_ref_device(device);

	return 0;
}



int pk2aux_init(void) {
	int rc;
	libusb_device **usb_devices = 0;
	ssize_t sz, i;

	/* Check if already initialized. */
	if (usb_context) {
		return LIBUSB_ERROR_BUSY;
	}

	/* Initialize libusb. */
	if ((rc = libusb_init(&usb_context)) < 0) {
		return rc;
	}

	/* Get the device list. */
	sz = libusb_get_device_list(usb_context, &usb_devices);
	if (sz < 0) {
		libusb_exit(usb_context);
		usb_context = 0;
		return sz;
	}

	/* Walk the devices looking for PICkit2s. */
	for (i = 0; i < sz; ++i) {
		if ((rc = examine_device(usb_devices[i])) < 0) {
			break;
		}
	}

	/* Free the list and those devices that were not reffed by examine_device. */
	libusb_free_device_list(usb_devices, 1);

	/* If examine_device failed for some device, call pk2aux_exit() and return the error code. */
	if (rc < 0) {
		pk2aux_exit();
		return rc;
	} else {
		return 0;
	}
}



void pk2aux_exit(void) {
	unsigned int i;

	for (i = 0; i < num_devices; ++i) {
		libusb_unref_device((libusb_device *) devices[i].private_data);
	}

	if (devices) {
		free(devices);
		devices = 0;
		num_devices = 0;
	}

	if (usb_context) {
		libusb_exit(usb_context);
		usb_context = 0;
	}
}



pk2aux_device_list pk2aux_get_devices(void) {
	pk2aux_device_list dlist;

	dlist.num_devices = num_devices;
	dlist.devices = devices;
	return dlist;
}



pk2aux_device *pk2aux_find_device(const char *path) {
	uint8_t bus_number, device_address;
	unsigned int i;

	/* Check if this is the NULL path case. */
	if (!path) {
		if (num_devices == 1) {
			return &devices[0];
		} else {
			return 0;
		}
	}

	/* Try to parse the path. */
	if (sscanf(path, "%" SCNu8 ":%" SCNu8, &bus_number, &device_address) != 2) {
		return 0;
	}

	/* Scan for the requested device. */
	for (i = 0; i < num_devices; i++) {
		if (devices[i].bus_number == bus_number && devices[i].device_address == device_address) {
			return &devices[i];
		}
	}

	return 0;
}



int pk2aux_open(pk2aux_device *device, pk2aux_handle *result) {
	pk2aux_handle handle;
	int rc, tmp_config;
	unsigned char buffer[64];

	/* Allocate space for the private data structure. */
	handle = malloc(sizeof(*handle));
	if (!handle) {
		return LIBUSB_ERROR_NO_MEM;
	}

	/* Open the PICkit2. */
	if ((rc = libusb_open((libusb_device *) device->private_data, &handle->usb_handle)) < 0) {
		free(handle);
		return rc;
	}

	/* Get its current configuration. */
	if ((rc = libusb_get_configuration(handle->usb_handle, &handle->original_configuration)) < 0) {
		libusb_close(handle->usb_handle);
		free(handle);
		return rc;
	}
	if (handle->original_configuration == 0) {
		handle->original_configuration = -1;
	}

	/* Set it to the non-HID configuration if needed. */
	if (handle->original_configuration != 2) {
		if ((rc = libusb_set_configuration(handle->usb_handle, 2)) < 0) {
			libusb_close(handle->usb_handle);
			free(handle);
			return rc;
		}
	}

	/* Claim the interface containing the two endpoints. */
	if ((rc = libusb_claim_interface(handle->usb_handle, 0)) < 0) {
		if (handle->original_configuration != 2) {
			libusb_set_configuration(handle->usb_handle, handle->original_configuration);
		}
		libusb_close(handle->usb_handle);
		free(handle);
		return rc;
	}

	/* Verify that we got the proper configuration. */
	if ((rc = libusb_get_configuration(handle->usb_handle, &tmp_config)) < 0) {
		libusb_release_interface(handle->usb_handle, 0);
		if (handle->original_configuration != 2) {
			libusb_set_configuration(handle->usb_handle, handle->original_configuration);
		}
		libusb_close(handle->usb_handle);
		free(handle);
		return rc;
	}
	if (tmp_config != 2) {
		libusb_release_interface(handle->usb_handle, 0);
		if (handle->original_configuration != 2) {
			libusb_set_configuration(handle->usb_handle, handle->original_configuration);
		}
		libusb_close(handle->usb_handle);
		free(handle);
		return LIBUSB_ERROR_BUSY;
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
	if ((rc = pk2aux_write(handle, buffer, 5)) < 0) {
		libusb_release_interface(handle->usb_handle, 0);
		if (handle->original_configuration != 2) {
			libusb_set_configuration(handle->usb_handle, handle->original_configuration);
		}
		libusb_close(handle->usb_handle);
		free(handle);
		return rc;
	}

	if ((rc = pk2aux_read(handle, buffer)) < 0) {
		libusb_release_interface(handle->usb_handle, 0);
		if (handle->original_configuration != 2) {
			libusb_set_configuration(handle->usb_handle, handle->original_configuration);
		}
		libusb_close(handle->usb_handle);
		free(handle);
		return rc;
	}

	assert(buffer[0] == 1);
	handle->pgc_floating = (buffer[buffer[0]] & 0x08) ? 1 : 0; /* PGC is RA3 */
	handle->pgd_floating = (buffer[buffer[0]] & 0x04) ? 1 : 0; /* PGD is RA2 */

	handle->uart_enabled = 0;

	*result = handle;
	return 0;
}



void pk2aux_reset(pk2aux_handle handle) {
	unsigned char buffer[1];

	if (handle->uart_enabled) {
		pk2aux_stop_uart(handle);
	}

	buffer[0] = RESET;
	pk2aux_write(handle, buffer, 1);
	libusb_reset_device(handle->usb_handle);
	libusb_close(handle->usb_handle);
	free(handle);
}



void pk2aux_close(pk2aux_handle handle) {
	if (handle->uart_enabled) {
		pk2aux_stop_uart(handle);
	}

	libusb_release_interface(handle->usb_handle, 0);
	if (handle->original_configuration != 2) {
		libusb_set_configuration(handle->usb_handle, handle->original_configuration);
	}
	libusb_close(handle->usb_handle);
	free(handle);
}



int pk2aux_get_version(pk2aux_handle handle, unsigned int *major, unsigned int *minor, unsigned int *micro) {
	unsigned char buffer[64];
	int rc;

	buffer[0] = FIRMWARE_VERSION;
	if ((rc = pk2aux_write(handle, buffer, 1)) < 0) {
		return rc;
	}

	if ((rc = pk2aux_read(handle, buffer)) < 0) {
		return rc;
	}

	*major = buffer[0];
	*minor = buffer[1];
	*micro = buffer[2];
	return 0;
}

