/******************************************************************
 *
 * Copyright (C) 2011 5V Technologies Ltd.
 * All Rights Reserved.
 *
 * This program is the proprietary software of 5V Technologies Ltd
 * and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from 5VT.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : 5VT OMCI protocol stack
 * Module  : gpon_hw_prx126
 * File    : gpon_hw_prx126_i2c.c
 *
 ******************************************************************/

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "util.h"
#include "gpon_sw.h"
#include "gpon_hw_prx126.h"
#include "intel_px126_pon.h"

int smbus_fp=-1;

static int gpon_hw_prx126_i2c_smbus_access(int fd, char read_write, unsigned char cmd,
				int size, union i2c_smbus_data *data)
{
	struct i2c_smbus_ioctl_data args;

	args.read_write = read_write;
	args.command = cmd;
	args.size = size;
	args.data = data;

	return ioctl(fd, I2C_SMBUS, &args);
}

static int gpon_hw_prx126_i2c_smbus_read_byte_data(int fd, unsigned char cmd)
{
	union i2c_smbus_data data;
	int err;

	err = gpon_hw_prx126_i2c_smbus_access(fd, I2C_SMBUS_READ, cmd,
				I2C_SMBUS_BYTE_DATA, &data);
	if (err < 0)
		return err;

	return data.byte;
}

static int32_t gpon_hw_prx126_i2c_smbus_write_byte_data(int file,
					 unsigned char cmd, unsigned char value)
{
	union i2c_smbus_data data;

	data.byte = value;

	return gpon_hw_prx126_i2c_smbus_access(file, I2C_SMBUS_WRITE, cmd,
				I2C_SMBUS_BYTE_DATA, &data);
}

int
gpon_hw_prx126_i2c_open(int i2c_port)
{
	char filename[64];

	sprintf(filename, "/dev/i2c-%d", i2c_port);
	smbus_fp = open(filename, O_RDWR);

	if (smbus_fp < 0) {
		return -1;
	}

	return smbus_fp;
}

int
gpon_hw_prx126_i2c_read(int i2c_port, unsigned int devaddr, unsigned short regaddr, void *buff, int len)
{
	int i, res;
	unsigned char *p = buff;

	if (smbus_fp < 0 || buff == NULL)
		return -1;
	
	/* Connect to the device */
	if (ioctl(smbus_fp, I2C_SLAVE, (void *)(int)devaddr) < 0) {
		return -1;
	}
	
	/* Get data from the device */
	for (i=0; i < len; i++) {
		res = gpon_hw_prx126_i2c_smbus_read_byte_data(smbus_fp, (unsigned char) (regaddr+i));
		if (res < 0) {
			return -1;
		}
		p[i] = (unsigned char)(res) & 0xff;
	}

	return 0;
}

int
gpon_hw_prx126_i2c_write(int i2c_port, unsigned int devaddr, unsigned short regaddr, void *buff, int len)
{
	int i,ret;
	unsigned char *p = buff;

	if (smbus_fp < 0 || buff == NULL)
		return -1;

	/* Connect to the device */
	if (ioctl(smbus_fp, I2C_SLAVE, (void *)(int)devaddr) < 0) {
		return -1;
	}

	/* Write data to the device */
	for (i=0; i < len; i++) {
		ret = gpon_hw_prx126_i2c_smbus_write_byte_data(smbus_fp, (unsigned char) (regaddr+i), p[i]);
		if (ret < 0) {
			return -1;
		}
	}

	return 0;
}

int
gpon_hw_prx126_i2c_close(int i2c_port)
{
	if (smbus_fp >= 0) {
		close(smbus_fp);
		smbus_fp = -1;
	}
	return 0;
}

int
gpon_hw_prx126_eeprom_open(int i2c_port, unsigned int devaddr)
{
	char filename[64];
	int ddmi_page=-1;
	int ret=-1;

	switch (devaddr)
	{
		case 0x4e:
			ddmi_page = 0; // A0
			break;
		case 0x4f:
			ddmi_page = 1; // A1
			break;
		default:
			return -1;
	}

	sprintf(filename, "/sys/bus/i2c/devices/%d-00%2x/eeprom", i2c_port, devaddr);

	//util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "file: %s\n)", filename);

	ret = intel_prx126_pon_eeprom_open(ddmi_page,filename);

	return ret;
}

int
gpon_hw_prx126_eeprom_data_get(int i2c_port, unsigned int devaddr, unsigned short regaddr, void *buff, int len)
{
	int ddmi_page=-1;
	int ret=-1;

	if (i2c_port) {}

	switch (devaddr)
	{
		case 0x4E:
			ddmi_page = 0; // A0
			break;
		case 0x4F:
			ddmi_page = 1; // A1
			break;
		default:
			return -1;
	}
	
	ret = intel_prx126_pon_eeprom_data_get(ddmi_page, (unsigned char *) buff, (long)regaddr, (size_t)len);

	return ret;
}

int
gpon_hw_prx126_eeprom_data_set(int i2c_port, unsigned int devaddr, unsigned short regaddr, void *buff, int len)
{
	int ddmi_page=-1;
	int ret=-1;

	if (i2c_port) {}

	switch (devaddr)
	{
		case 0x4E:
			ddmi_page = 0; // A0
			break;
		case 0x4F:
			ddmi_page = 1; // A1
			break;
		default:
			return -1;
	}

	ret = intel_prx126_pon_eeprom_data_set(ddmi_page, (unsigned char *)buff, (long) regaddr, (size_t) len);

	return ret;
}

