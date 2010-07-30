#if !defined CMD_H
#define CMD_H

enum PK2_CMDS {
	ENTER_BOOTLOADER = 0x42,
	NO_OPERATION = 0x5A,
	FIRMWARE_VERSION = 0x76,
	SETVDD = 0xA0,
	SETVPP,
	READ_STATUS,
	READ_VOLTAGES,
	DOWNLOAD_SCRIPT,
	RUN_SCRIPT,
	EXECUTE_SCRIPT,
	CLR_DOWNLOAD_BUFFER,
	DOWNLOAD_DATA,
	CLR_UPLOAD_BUFFER,
	UPLOAD_DATA,
	CLR_SCRIPT_BUFFER,
	UPLOAD_DATA_NOLEN,
	END_OF_BUFFER,
	RESET,
	SCRIPT_BUFFER_CHKSUM,
	SET_VOLTAGE_CALS,
	WR_INTERNAL_EE,
	RD_INTERNAL_EE,
	ENTER_UART_MODE,
	EXIT_UART_MODE,
	ENTER_LEARN_MODE,
	EXIT_LEARN_MODE,
	ENABLE_PK2GO_MODE,
	LOGIC_ANALYZER_GO,
	COPY_RAM_UPLOAD,
	READ_OSCCAL = 0x80,
	WRITE_OSCCAL,
	START_CHECKSUM,
	VERIFY_CHECKSUM,
	CHECK_DEVICE_ID,
	READ_BANDGAP,
	WRITE_CFG_BANDGAP,
	CHANGE_CHKSM_FRMT
};

enum PK2_SCRIPT_CMDS {
	JT2_PE_PROG_RESP = 0xB3,
	JT2_WAIT_PE_RESP,
	JT2_GET_PE_RESP,
	JT2_XFERINST_BUF,
	JT2_XFRFASTDAT_BUF,
	JT2_XFRFASTDAT_LIT,
	JT2_XFERDATA32_LIT,
	JT2_XFERDATA8_LIT,
	JT2_SENDCMD,
	JT2_SETMODE,
	UNIO_TX_RX,
	UNIO_TX,
	MEASURE_PULSE,
	ICDSLAVE_TX_BUF_BL,
	ICDSLAVE_TX_LIT_BL,
	ICDSLAVE_RX_BL,
	SPI_RDWR_BYTE_BUF,
	SPI_RDWR_BYTE_LIT,
	SPI_RD_BYTE_BUF,
	SPI_WR_BYTE_BUF,
	SPI_WR_BYTE_LIT,
	I2C_RD_BYTE_NACK,
	I2C_RD_BYTE_ACK,
	I2C_WR_BYTE_BUF,
	I2C_WR_BYTE_LIT,
	I2C_STOP,
	I2C_START,
	AUX_STATE_BUFFER,
	SET_AUX,
	WRITE_BITS_BUF_HLD,
	WRITE_BITS_LIT_HLD,
	CONST_WRITE_DL,
	WRITE_BUFBYTE_W,
	WRITE_BUFWORD_W,
	RD2_BITS_BUFFER,
	RD2_BYTE_BUFFER,
	VISI24,
	NOP24,
	COREINST24,
	COREINST18,
	POP_DOWNLOAD,
	ICSP_STATES_BUFFER,
	LOOPBUFFER,
	ICDSLAVE_TX_BUF,
	ICDSLAVE_TX_LIT,
	ICDSLAVE_RX,
	POKE_SFR,
	PEEK_SFR,
	EXIT_SCRIPT,
	GOTO_INDEX,
	IF_GT_GOTO,
	IF_EQ_GOTO,
	DELAY_SHORT,
	DELAY_LONG,
	LOOP,
	SET_ICSP_SPEED,
	READ_BITS,
	READ_BITS_BUFFER,
	WRITE_BITS_BUFFER,
	WRITE_BITS_LITERAL,
	READ_BYTE,
	READ_BYTE_BUFFER,
	WRITE_BYTE_BUFFER,
	WRITE_BYTE_LITERAL,
	SET_ICSP_PINS,
	BUSY_LED_OFF,
	BUSY_LED_ON,
	MCLR_GND_OFF,
	MCLR_GND_ON,
	VPP_PWM_OFF,
	VPP_PWM_ON,
	VPP_OFF,
	VPP_ON,
	VDD_GND_OFF,
	VDD_GND_ON,
	VDD_OFF,
	VDD_ON
};

#endif

