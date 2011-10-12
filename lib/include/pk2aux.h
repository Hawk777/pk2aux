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
#if !defined PK2AUX_H
#define PK2AUX_H

/**
 * \file
 *
 * \brief Defines the public API that programs can use to call into the library.
 */

#include <stddef.h>
#include <stdint.h>
#include <limits.h>



/**
 * \brief Describes one of the PICkit2 devices connected to the system.
 */
typedef struct pk2aux_device {
	/**
	 * \brief The unit ID string burned into the device.
	 *
	 * Set to a zero-length string if no unit ID is burned in.
	 */
	char unit_id[16];

	/**
	 * \brief The USB bus number where the device is attached.
	 */
	uint8_t bus_number;

	/**
	 * \brief The USB address of the device on its bus.
	 */
	uint8_t device_address;

	/**
	 * \brief A pointer to private data used internally by libpk2aux.
	 */
	void *private_data;
} pk2aux_device;



/**
 * \brief A collection of zero or more PICkit2 devices.
 */
typedef struct pk2aux_device_list {
	/**
	 * \brief The number of devices.
	 */
	unsigned int num_devices;

	/**
	 * \brief The devices.
	 */
	pk2aux_device *devices;
} pk2aux_device_list;



/**
 * \brief A handle to an open PICkit2.
 *
 * Handles of this type are used to operate on a PICkit2.
 */
typedef struct pk2aux_handle_impl *pk2aux_handle;



/**
 * \brief A mode into which a pin can be placed.
 */
enum PIN_MODE {
	/**
	 * \brief The pin is driven to ground.
	 *
	 * See the individual pin-control functions for precise electrical details.
	 */
	PIN_MODE_GROUNDED,

	/**
	 * \brief The pin is not driven.
	 *
	 * See the individual pin-control functions for precise electrical details.
	 */
	PIN_MODE_FLOATING,

	/**
	 * \brief The pin is driven high.
	 *
	 * See the individual pin-control functions for precise electrical details.
	 */
	PIN_MODE_HIGH
};



/**
 * \brief Initializes libpk2aux and scans the system for PICkit2 devices.
 *
 * This function must be called to initialize the library before any other functions are called.
 *
 * \return 0 on success or a libusb error code on failure.
 */
int pk2aux_init(void);



/**
 * \brief Deinitializes libpk2aux.
 *
 * This function must be called after any other functions.
 */
void pk2aux_exit(void);



/**
 * \brief Returns the array of located PICkit2 devices.
 *
 * \return the array of found programmers.
 */
pk2aux_device_list pk2aux_get_devices(void);



/**
 * \brief Searches the scanned list of PICkit2 devices for the device at the given path.
 *
 * \param[in] path the path of the device to look for, in the form <code>"bus_number:device_address"</code>,
 * or null to find the only device on the system (or fail if multiple devices are attached).
 *
 * \return the device on success or null on failure.
 */
pk2aux_device *pk2aux_find_device(const char *path);



/**
 * \brief Opens a PICkit2.
 *
 * \param[in] device the device to open.
 *
 * \param[out] handle the device handle that can be used to operate on the device.
 *
 * \return 0 on success or a libusb error code on failure.
 */
int pk2aux_open(pk2aux_device *device, pk2aux_handle *handle);



/**
 * \brief Closes an open handle.
 *
 * \param[in] handle the handle to close, which becomes invalid.
 */
void pk2aux_close(pk2aux_handle handle);



/**
 * \brief Resets the PICkit2.
 *
 * \param[in] handle the handle of the device to reset, which becomes invalid.
 */
void pk2aux_reset(pk2aux_handle handle);



/**
 * \brief Gets the firmware version in the device.
 *
 * \param[in] handle the handle of the device to inspect.
 *
 * \param[out] major the major version of the firmware.
 *
 * \param[out] minor the minor version of the firmware.
 *
 * \param[out] micro the micro version of the firmware.
 *
 * \return 0 on success or a libusb error code on failure.
 */
int pk2aux_get_version(pk2aux_handle handle, unsigned int *major, unsigned int *minor, unsigned int *micro);



/**
 * \brief Sets the unit ID.
 *
 * \param[in] handle the handle of the device whose unit ID should be set.
 *
 * \param[in] id the ID to set, which may be up to 15 characters in length or may be null to remove the unit ID.
 *
 * \return 0 on success or a libusb error code on failure.
 */
int pk2aux_set_id(pk2aux_handle handle, const char *id);



/**
 * \brief Configures the VDD pin.
 *
 * The VDD pin can be:
 * \li driven hard to ground through a transistor (\ref PIN_MODE_GROUNDED),
 * \li allowed to float and be driven by the target circuit (\ref PIN_MODE_FLOATING), or
 * \li driven hard to the output of a linear regulator (\ref PIN_MODE_HIGH).
 *
 * \param[in] handle the handle of the device to modify.
 *
 * \param[in] mode the pin mode to set.
 *
 * \return 0 on success or a libusb error code on failure.
 */
int pk2aux_set_vdd_mode(pk2aux_handle handle, enum PIN_MODE mode);



/**
 * \brief Sets the voltage generated by the VDD linear regulator.
 *
 * The regulator drives the pin when the pin is set (via pk2aux_set_vdd_mode()) to \ref PIN_MODE_HIGH.
 *
 * \param[in] handle the handle of the device to modify.
 *
 * \param[in] voltage the voltage to set.
 *
 * \return 0 on success or a libusb error code on failure.
 */
int pk2aux_set_vdd_level(pk2aux_handle handle, double voltage);



/**
 * \brief Measures the voltage on the VDD pin.
 *
 * This may be generated by the linear regulator, be provided by the external circuit, or be ground.
 *
 * \param[in] handle the handle of the device to inspect.
 *
 * \param[out] voltage the measured voltage.
 *
 * \return 0 on success or a libusb error code on failure.
 */
int pk2aux_get_vdd_level(pk2aux_handle handle, double *voltage);



/**
 * \brief Configures the VPP pin.
 *
 * The VPP pin can be:
 * \li driven hard to ground through a transistor (\ref PIN_MODE_GROUNDED),
 * \li allowed to float and be driven by the target circuit (\ref PIN_MODE_FLOATING), or
 * \li attached to the output of a boost converter (\ref PIN_MODE_HIGH).
 *
 * \param[in] handle the handle of the device to modify.
 *
 * \param[in] mode the pin mode to set.
 *
 * \return 0 on success or a libusb error code on failure.
 */
int pk2aux_set_vpp_mode(pk2aux_handle handle, enum PIN_MODE mode);



/**
 * \brief Sets the voltage generated by the VPP boost converter.
 *
 * The pump drives the pin when the pin is set (via pk2aux_set_vpp_mode()) to \ref PIN_MODE_HIGH.
 * A delay of 100ms should be given to allow the converter to stabilize before using its output.
 * The converter is powered by the VDD linear regulator.
 * If the VDD regulator's output is fairly low, some higher VPP levels may be impossible to generate.
 * The converter also cannot output a voltage below that of the VDD regulator.
 *
 * \param[in] handle the handle of the device to modify.
 *
 * \param[in] voltage the voltage to set, which cannot be less than the output of the VDD linear regulator.
 *
 * \return 0 on success or a libusb error code on failure.
 */
int pk2aux_set_vpp_level(pk2aux_handle handle, double voltage);



/**
 * \brief Shuts down the VPP boost converter.
 *
 * When the converter is shut down, its output is approximately equal to the output of the VDD regulator.
 * Shutting down the converter saves power if high voltages are not needed on VPP.
 *
 * \param[in] handle the handle of the device to modify.
 *
 * \return 0 on success or a libusb error code on failure.
 */
int pk2aux_stop_vpp_pump(pk2aux_handle handle);



/**
 * \brief Measures the voltage at the output of the VPP boost converter.
 *
 * \param[in] handle the handle of the device to inspect.
 *
 * \param[out] voltage the measured voltage.
 *
 * \return 0 on success or a libusb error code on failure.
 */
int pk2aux_get_vpp_level(pk2aux_handle handle, double *voltage);



/**
 * \brief Sets the mode of the PGC pin.
 *
 * The PGC pin can be:
 * \li driven to ground through a transistor and a resistor (\ref PIN_MODE_GROUNDED),
 * \li allowed to float clamped to VDD and be driven by the target circuit (\ref PIN_MODE_FLOATING), or
 * \li driven to VDD through a transistor and a resistor (\ref PIN_MODE_HIGH).
 *
 * \param[in] handle the handle of the device to modify.
 *
 * \param[in] mode the pin mode to set.
 *
 * \return 0 on success or a libusb error code on failure.
 */
int pk2aux_set_pgc(pk2aux_handle handle, enum PIN_MODE mode);



/**
 * \brief Sets the mode of the PGD pin.
 *
 * The PGD pin can be:
 * \li driven to ground through a transistor and a resistor (\ref PIN_MODE_GROUNDED),
 * \li allowed to float clamped to VDD and be driven by the target circuit (\ref PIN_MODE_FLOATING), or
 * \li driven to VDD through a transistor and a resistor (\ref PIN_MODE_HIGH).
 *
 * \param[in] handle the handle of the device to modify.
 *
 * \param[in] mode the pin mode to set.
 *
 * \return 0 on success or a libusb error code on failure.
 */
int pk2aux_set_pgd(pk2aux_handle handle, enum PIN_MODE mode);



/**
 * \brief Sets the mode of the AUX pin.
 *
 * The AUX pin can be:
 * \li driven to ground through a transistor and a resistor (\ref PIN_MODE_GROUNDED),
 * \li allowed to float clamped to VDD and be driven by the target circuit (\ref PIN_MODE_FLOATING), or
 * \li driven to VDD through a transistor and a resistor (\ref PIN_MODE_HIGH).
 *
 * \param[in] handle the handle of the device to modify.
 *
 * \param[in] mode the pin mode to set.
 *
 * \return 0 on success or a libusb error code on failure.
 */
int pk2aux_set_aux(pk2aux_handle handle, enum PIN_MODE mode);



/**
 * \brief Gets the logic level of the PGC pin.
 *
 * If the PICkit2 is driving the pin, the driven polarity is returned even if the voltage is close to zero due to the VDD clamp.
 *
 * \param[in] handle the handle of the device to inspect.
 *
 * \param[out] level 0 if the pin is low, or 1 if it is high.
 *
 * \return 0 on success or a libusb error code on failure.
 */
int pk2aux_get_pgc(pk2aux_handle handle, unsigned int *level);



/**
 * \brief Gets the logic level of the PGD pin.
 *
 * If the PICkit2 is driving the pin, the driven polarity is returned even if the voltage is close to zero due to the VDD clamp.
 *
 * \param[in] handle the handle of the device to inspect.
 *
 * \param[out] level 0 if the pin is low, or 1 if it is high.
 *
 * \return 0 on success or a libusb error code on failure.
 */
int pk2aux_get_pgd(pk2aux_handle handle, unsigned int *level);



/**
 * \brief Gets the logic level of the AUX pin.
 *
 * If the PICkit2 is driving the pin, the driven polarity is returned even if the voltage is close to zero due to the VDD clamp.
 *
 * \param[in] handle the handle of the device to inspect.
 *
 * \param[out] level 0 if the pin is low, or 1 if it is high.
 *
 * \return 0 on success or a libusb error code on failure.
 */
int pk2aux_get_aux(pk2aux_handle handle, unsigned int *level);



/**
 * \brief Initiates UART mode.
 *
 * \param[in] handle the handle of the device to modify.
 *
 * \param[in] baud the baud rate at which to send and receive data.
 *
 * \return 0 on success or a libusb error code on failure.
 */
int pk2aux_start_uart(pk2aux_handle handle, unsigned int baud);



/**
 * \brief Exits UART mode.
 *
 * \param[in] handle the handle of the device to modify.
 *
 * \return 0 on success or a libusb error code on failure.
 */
int pk2aux_stop_uart(pk2aux_handle handle);



/**
 * \brief Retrieves received data from the UART.
 *
 * \param[in] handle the handle of the device from which to read data.
 *
 * \param[out] buffer the buffer into which to store the data.
 *
 * \param[in,out] length on call, the amount of space in \p buffer; on return, the number of bytes returned.
 *
 * \return 0 on success or a libusb error code on failure.
 */
int pk2aux_receive_uart(pk2aux_handle handle, void *buffer, size_t *length);



/**
 * \brief Sends data to the UART.
 *
 * \param[in] handle the handle of the device to which to send data.
 *
 * \param[in] buffer the data to send.
 *
 * \param[in] length the number of bytes to send.
 *
 * \return 0 on success or a libusb error code on failure.
 */
int pk2aux_send_uart(pk2aux_handle handle, const void *buffer, size_t length);



/**
 * \brief Returns a string error message corresponding to a libusb error code.
 *
 * \return the error message, in a static buffer which should not be modified or freed.
 */
const char *pk2aux_error_string(int rc);

#endif

