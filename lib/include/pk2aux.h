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
#if !defined PK2AUX_H
#define PK2AUX_H

#include <stddef.h>
#include <limits.h>

/*
 * Describes one of the PICkit2 devices connected to the system.
 */
typedef struct pk2aux_device {
	/* The unit ID string burned into the device, or a zero-length
	 * string if no unit ID is burned in. */
	char unit_id[16];

	/* The "path" of the device on the USB. */
	char usb_path[PATH_MAX * 2 + 2];

	/* A pointer to private data used internally by libpk2aux. */
	void *private_data;
} pk2aux_device;



/*
 * A collection of zero or more PICkit2 devices.
 */
typedef struct pk2aux_device_list {
	/* The number of devices. */
	unsigned int num_devices;

	/* The devices. */
	pk2aux_device *devices;
} pk2aux_device_list;



/*
 * A handle to an open PICkit2.
 */
typedef struct pk2aux_handle_impl *pk2aux_handle;



/*
 * A mode into which a power pin can be placed.
 */
enum PIN_MODE {
	/* Driven to ground. */
	PIN_MODE_GROUNDED,
	/* Completely unconnected. */
	PIN_MODE_FLOATING,
	/* Connected to the positive voltage. */
	PIN_MODE_HIGH
};



/*
 * Scans the system for PICkit2 devices. This function must be called before any others.
 *
 * Returns 0 on success, negative on error.
 */
int pk2aux_scan(void);



/*
 * Returns the array of located PICkit2 devices.
 *
 * Returns the array of found programmers.
 */
pk2aux_device_list pk2aux_get_devices(void);



/*
 * Searches the scanned list of PICkit2 devices for the device at the specified path.
 * Provide a pathname equal to the path in one of the pk2aux_device structures to find
 * that device. Providing an empty string will succeed if only one PICkit2 is attached
 * to the system, returning that sole device.
 *
 * Returns: the device on success, NULL on failure
 */
pk2aux_device *pk2aux_find_device(const char *path);



/*
 * Opens a PICkit2.
 *
 * Returns a handle on success, NULL on failure.
 */
pk2aux_handle pk2aux_open(pk2aux_device *device);



/*
 * Resets the PICkit2. The handle is also closed; DO NOT call pk2aux_close().
 */
void pk2aux_reset(pk2aux_handle handle);



/*
 * Closes an open PICkit2 handle.
 */
void pk2aux_close(pk2aux_handle handle);



/*
 * Gets the firmware version in the device.
 *
 * Returns 0 on succes, -1 on failure.
 */
int pk2aux_get_version(pk2aux_handle handle, unsigned int *major, unsigned int *minor, unsigned int *micro);



/*
 * Sets the unid ID of a PICkit2. The unit ID may be up to 15
 * characters in length. Pass a NULL pointer to remove the unit
 * ID. This is not the same as setting the unit ID to an empty string,
 * though the two situations are indistinguishable with respect to the
 * unid_id field in the pk2aux_device structure.
 *
 * Returns 0 on success, -1 on failure.
 */
int pk2aux_set_id(pk2aux_handle handle, const char *id);



/*
 * Configures the VDD pin of the PICkit2. The pin may be set to grounded,
 * floating, or high. If the pin is set high, it will reflect the voltage set
 * in the most recent call to pk2aux_set_vdd_level() or a PICkit2-defined default.
 *
 * Returns 0 on success, -1 on failure.
 */
int pk2aux_set_vdd_mode(pk2aux_handle handle, enum PIN_MODE mode);



/*
 * Sets the voltage on the VDD pin of the PICkit2. The voltage level of VDD may
 * be set independently to the power mode. It may be wise to allow a short delay
 * for the voltage generator to stabilize before calling pk2aux_set_vdd_mode() to
 * connect the voltage to the target circuit.
 *
 * Returns 0 on success, -1 on failure.
 */
int pk2aux_set_vdd_level(pk2aux_handle handle, double voltage);



/*
 * Measures the VDD voltage. This is the voltage actually present on the pin, which
 * may be produced by the PICkit2's VDD generator, may be produced by the grounding
 * circuit, or may be produced by the target circuit (if the pin is set floating).
 *
 * Returns 0 on success, -1 on failure.
 */
int pk2aux_get_vdd_level(pk2aux_handle handle, double *voltage);



/*
 * Configures the VPP pin of the PICkit2. The pin may be set to grounded,
 * floating, or high. If the pin is set high, it will reflect the voltage set
 * in the most recent call to pk2aux_set_vpp_level(), or drive near VDD if the
 * charge pump is shut down.
 *
 * Returns 0 on success, -1 on failure.
 */
int pk2aux_set_vpp_mode(pk2aux_handle handle, enum PIN_MODE mode);



/*
 * Enables the VPP charge pump and sets the voltage it should generate. The voltage
 * level of VPP may be set independently to the power mode. It is suggested to allow
 * at least 100ms for the charge pump to stabilize before calling pk2aux_set_vpp_mode()
 * to connect the voltage to the target circuit, especially if the charge pump was
 * previously shut down.
 *
 * Be aware that the VPP charge pump is supplied with power from the VDD voltage generator.
 * If the VDD level is very low, some of the higher VPP levels may not be available.
 * Additionally, it is not possible to set VPP less than VDD.
 *
 * Returns 0 on success, -1 on failure.
 */
int pk2aux_set_vpp_level(pk2aux_handle handle, double voltage);



/*
 * Shuts down the VPP charge pump. When the pump is shut down, setting the VPP pin high
 * will drive near VDD. Shutting down the charge pump will save power if VPP is not needed
 * for a period of time. It is highly recommended to use pk2aux_set_vpp_mode() to set the
 * mode to either floating or grounded before shutting down the charge pump.
 */
int pk2aux_stop_vpp_pump(pk2aux_handle handle);



/*
 * Measures the VPP voltage. This is the voltage coming out of the charge pump, which is
 * not always the same as the voltage at the VPP pin (it will be different if the pin is
 * grounded or floating).
 *
 * Returns 0 on success, -1 on failure.
 */
int pk2aux_get_vpp_level(pk2aux_handle handle, double *voltage);



/*
 * Sets the mode of the PGC pin. The pin can be set to any of grounded, floating, or high.
 *
 * Returns 0 on success, -1 on failure.
 */
int pk2aux_set_pgc(pk2aux_handle handle, enum PIN_MODE mode);



/*
 * Sets the mode of the PGD pin. The pin can be set to any of grounded, floating, or high.
 *
 * Returns 0 on success, -1 on failure.
 */
int pk2aux_set_pgd(pk2aux_handle handle, enum PIN_MODE mode);



/*
 * Sets the mode of the AUX pin. The pin can be set to any of grounded, floating, or high.
 *
 * Returns 0 on success, -1 on failure.
 */
int pk2aux_set_aux(pk2aux_handle handle, enum PIN_MODE mode);



/*
 * Gets the level of the PGC pin. This will be either high or low, represented by either 1
 * or 0 begin written to *level. This function does not differentiate between whether the
 * pin is an input or an output. If the pin is an input, the returned value is the sampled
 * level presented by the external circuit; if the pin is an output, the returned value is
 * the level driven by the PICkit2.
 *
 * Returns 0 on success, -1 on failure.
 */
int pk2aux_get_pgc(pk2aux_handle handle, unsigned int *level);



/*
 * Gets the level of the PGD pin. This will be either high or low, represented by either 1
 * or 0 begin written to *level. This function does not differentiate between whether the
 * pin is an input or an output. If the pin is an input, the returned value is the sampled
 * level presented by the external circuit; if the pin is an output, the returned value is
 * the level driven by the PICkit2.
 *
 * Returns 0 on success, -1 on failure.
 */
int pk2aux_get_pgd(pk2aux_handle handle, unsigned int *level);



/*
 * Gets the level of the AUX pin. This will be either high or low, represented by either 1
 * or 0 begin written to *level. This function does not differentiate between whether the
 * pin is an input or an output. If the pin is an input, the returned value is the sampled
 * level presented by the external circuit; if the pin is an output, the returned value is
 * the level driven by the PICkit2.
 *
 * Returns 0 on success, -1 on failure.
 */
int pk2aux_get_aux(pk2aux_handle handle, unsigned int *level);



/*
 * Initiates UART mode on the PICkit2 with the given baud rate.
 *
 * Returns 0 on success, -1 on failure.
 */
int pk2aux_start_uart(pk2aux_handle handle, unsigned int baud);



/*
 * Stops UART mode on the PICkit2.
 *
 * Returns 0 on success, -1 on failure.
 */
int pk2aux_stop_uart(pk2aux_handle handle);



/*
 * Retrieves received data from the UART. The data is stored into *buffer, which
 * must be of length *length. *length is updated to indicate the amount of data
 * stored.
 *
 * Returns 0 on success, -1 on failure.
 */
int pk2aux_receive_uart(pk2aux_handle handle, void *buffer, size_t *length);



/*
 * Sends data to the UART. The data at *buffer, of length length, will be sent.
 * This function will not return until all the data has been consumed or an error
 * occurs.
 *
 * Returns 0 on success, -1 on failure.
 */
int pk2aux_send_uart(pk2aux_handle handle, const void *buffer, size_t length);

#endif

