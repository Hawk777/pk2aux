#include "pk2aux.h"
#include "internal.h"
#include "cmd.h"
#include <errno.h>
#include <math.h>
#include <assert.h>



static int get_voltages(pk2aux_handle handle, double *vdd, double *vpp) {
	unsigned char buffer[64];

	buffer[0] = READ_VOLTAGES;
	if (pk2aux_write(handle, buffer, 1) < 0)
		return -1;

	if (pk2aux_read(handle, buffer) < 0)
		return -1;

	*vdd = (buffer[0] + buffer[1] * 256) * 5.0 / 65536.0;
	*vpp = (buffer[2] + buffer[3] * 256) * 13.7 / 65536.0;
	return 0;
}



int pk2aux_set_vdd_mode(pk2aux_handle handle, enum PIN_MODE mode) {
	unsigned char buffer[4];

	/* Need to execute a script in order to set the VDD mode. */
	buffer[0] = EXECUTE_SCRIPT;
	buffer[1] = 2;

	/* Careful, always turn off a transistor before turning the other on.
	 * Doesn't actually matter as there are resisters, but doesn't hurt. */
	switch (mode) {
		case PIN_MODE_GROUNDED:
			buffer[2] = VDD_OFF;
			buffer[3] = VDD_GND_ON;
			break;

		case PIN_MODE_FLOATING:
			buffer[2] = VDD_OFF;
			buffer[3] = VDD_GND_OFF;
			break;

		case PIN_MODE_HIGH:
			buffer[2] = VDD_GND_OFF;
			buffer[3] = VDD_ON;
			break;

		default:
			errno = ENOSYS;
			return -1;
	}

	if (pk2aux_write(handle, buffer, 4) < 0)
		return -1;

	return 0;
}



int pk2aux_set_vdd_level(pk2aux_handle handle, double voltage) {
	unsigned char buffer[4];
	unsigned int ccpr;
	unsigned int fault;

	/* Check for a sensible voltage level. */
	if (voltage < 0.0 || voltage > 5.0) {
		errno = EDOM;
		return -1;
	}

	/* Compute the duty cycle. Careful of rounding (hence the +0.5). */
	ccpr = ((unsigned int) (((voltage * 32.0) + 10.5) + 0.5)) << 6;
	assert(ccpr < 65536);

	/* Compute the fault level. Careful of rounding again. */
	fault = (unsigned int) ((((voltage * 0.7) / 5.0) * 255.0) + 0.5);
	assert(fault < 256);

	/* Send the command. */
	buffer[0] = SETVDD;
	buffer[1] = (unsigned char) (ccpr & 0xFF);
	buffer[2] = (unsigned char) (ccpr >> 8);
	buffer[3] = (unsigned char) fault;

	if (pk2aux_write(handle, buffer, 4) < 0)
		return -1;

	return 0;
}



int pk2aux_get_vdd_level(pk2aux_handle handle, double *voltage) {
	double vpp;
	return get_voltages(handle, voltage, &vpp);
}



int pk2aux_set_vpp_mode(pk2aux_handle handle, enum PIN_MODE mode) {
	unsigned char buffer[4];

	/* Need to execute a script in order to set the VPP mode. */
	buffer[0] = EXECUTE_SCRIPT;
	buffer[1] = 2;

	/* Careful, always turn off a transistor before turning the other on.
	 * Doesn't actually matter as there are resisters, but doesn't hurt. */
	switch (mode) {
		case PIN_MODE_GROUNDED:
			buffer[2] = VPP_OFF;
			buffer[3] = MCLR_GND_ON;
			break;

		case PIN_MODE_FLOATING:
			buffer[2] = VPP_OFF;
			buffer[3] = MCLR_GND_OFF;
			break;

		case PIN_MODE_HIGH:
			buffer[2] = MCLR_GND_OFF;
			buffer[3] = VPP_ON;
			break;

		default:
			errno = ENOSYS;
			return -1;
	}

	if (pk2aux_write(handle, buffer, 4) < 0)
		return -1;

	return 0;
}



int pk2aux_set_vpp_level(pk2aux_handle handle, double voltage) {
	unsigned int adc;
	unsigned int fault;
	unsigned char buffer[7];

	/* Check for a sensible voltage level. */
	if (voltage < 0.0 || voltage > 13.7) {
		errno = EDOM;
		return -1;
	}

	/* Compute the ADC target level. Careful of rounding (hence the +0.5). */
	adc = (unsigned int) ((voltage * 18.61) + 0.5);
	assert(adc < 255);

	/* Compute the fault level. Careful of rounding again. */
	fault = (unsigned int) ((voltage * 0.7 * 18.61) + 0.5);
	assert(fault < 255);

	/* We need to not only set the level, but also turn on the charge pump. */
	buffer[0] = EXECUTE_SCRIPT;
	buffer[1] = 1;
	buffer[2] = VPP_PWM_ON;
	buffer[3] = SETVPP;
	buffer[4] = 0x40;
	buffer[5] = (unsigned char) adc;
	buffer[6] = (unsigned char) fault;
	if (pk2aux_write(handle, buffer, 7) < 0)
		return -1;

	return 0;
}



int pk2aux_stop_vpp_pump(pk2aux_handle handle) {
	unsigned char buffer[3];

	/* This must be done as a script. */
	buffer[0] = EXECUTE_SCRIPT;
	buffer[1] = 1;
	buffer[2] = VPP_PWM_OFF;
	if (pk2aux_write(handle, buffer, 3) < 0)
		return -1;

	return 0;
}



int pk2aux_get_vpp_level(pk2aux_handle handle, double *voltage) {
	double vdd;
	return get_voltages(handle, &vdd, voltage);
}
