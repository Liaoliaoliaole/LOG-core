/*
File: Morfeas_RPi_Hat.c  Implementation of functions related to Morfeas_RPi_Hat.
Copyright (C) 12019-12020  Sam harry Tzavaras

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License, or any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <asm/ioctl.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>

#include "../Morfeas_Types.h"
#include "Morfeas_RPi_Hat.h"

	//---- LEDs related ----//
//Init Morfeas_RPi_Hat LEDs, return 1 if sysfs files exist, 0 otherwise.
int led_init(char *CAN_IF_name)
{
	char path[35];
	char buffer[3];
	ssize_t bytes_written;
	int sysfs_fd, i, pin;
	if(!strcmp(CAN_IF_name, "can0") || !strcmp(CAN_IF_name, "can1"))
	{	//init GPIO on sysfs
		for(i=0; i<2; i++)
		{
			sysfs_fd = open("/sys/class/gpio/export", O_WRONLY);
			if(sysfs_fd < 0)
			{
				fprintf(stderr, "LEDs are Not supported!\n");
				return 0;
			}
			pin = i ? YELLOW_LED : RED_LED;
			bytes_written = snprintf(buffer, 3, "%d", pin);
			write(sysfs_fd, buffer, bytes_written);
			close(sysfs_fd);
		}
		sleep(1);
		//Set direction of GPIOs
		for(i=0; i<2; i++)
		{
			pin = i ? YELLOW_LED : RED_LED;
			snprintf(path, 35, "/sys/class/gpio/gpio%d/direction", pin);
			sysfs_fd = open(path, O_WRONLY);
			if(sysfs_fd < 0)
			{
				fprintf(stderr, "LEDs are Not supported! (Direction File Error!!!)\n");
				return 0;
			}
			if (write(sysfs_fd, "out", 3)<0)
			{
				fprintf(stderr, "Failed to set direction!\n");
				return 0;
			}
			close(sysfs_fd);

		}
	}
	return 1;
}
//Write value to Morfeas_RPi_Hat LED, return 0 if write was success, -1 otherwise.
int GPIOWrite(int LED_name, int value)
{
	static const char s_values_str[] = "01";
	char path[50];
	int fd;
	snprintf(path, 30, "/sys/class/gpio/gpio%d/value", LED_name);
	fd = open(path, O_WRONLY);
	if (-1 == fd)
	{
		fprintf(stderr, "Failed to open gpio value for writing!\n");
		return(-1);
	}
	if (1 != write(fd, &s_values_str[!value ? 0 : 1], 1))
	{
		fprintf(stderr, "Failed to write value!\n");
		return(-1);
	}
	close(fd);
	return(0);
}
//Read value of Morfeas_RPi_Hat LED by name, return value of the LED, or -1 if read failed.
int GPIORead(int LED_name)
{
	char read_val[30] = {0};
	char path[50];
	int fd;
	snprintf(path, 30, "/sys/class/gpio/gpio%d/value", LED_name);
	fd = open(path, O_WRONLY);
	if (-1 == fd)
	{
		fprintf(stderr, "Failed to open gpio value for writing!\n");
		return(-1);
	}
	if (read(fd, &read_val, 1) != 1)
	{
		fprintf(stderr, "Failed to write value!\n");
		return(-1);
	}
	close(fd);
	return(atoi(read_val));
}
	//---- I2C device related ----//
//Function to init the MAX9611
int MAX9611_init(unsigned char port, unsigned char i2c_dev_num)
{
	return 0;
}
//Function that read measurements for MAX9611, store them on memory pointed by meas.
int get_port_meas(struct Morfeas_RPi_Hat_Port_meas *meas, unsigned char port, unsigned char i2c_dev_num)
{
	char filename[30];//Path to sysfs I2C-dev
	int i2c_fd;//I2C file descriptor
	int addr;// Address for MAX9611 connected to port
	switch(port)
	{
		case 0: addr=0x70; break;
		case 1: addr=0x73; break;
		case 2: addr=0x7c; break;
		case 3: addr=0x7f; break;
		default: return -1;
	}
	//Open I2C-bus
	sprintf(filename, "/dev/i2c-%u", i2c_dev_num);
	i2c_fd = open(filename, O_RDWR);
	if (i2c_fd < 0)
	{
	  perror("Error on I2C open:");
	  return -1;
	}
	if (ioctl(i2c_fd, I2C_SLAVE, addr) < 0)
	{
	  perror("Error on ioctl:");
	  close(i2c_fd);
	  return -1;
	}
	//read the measurements
	i2c_smbus_read_block_data(i2c_fd, 0, (unsigned char*)meas);
	//write(i2c_fd, buf, 1);
	close(i2c_fd);
	return 0;
}
//Function that read data from EEPROM
int read_port_config(struct Morfeas_RPi_Hat_EEPROM_CANBus_Port_config *config, unsigned char port, unsigned char i2c_dev_num)
{
	return 0;
}
//Function that write data to EEPROM, checksum calculated inside.
int write_port_config(struct Morfeas_RPi_Hat_EEPROM_CANBus_Port_config *config, unsigned char port, unsigned char i2c_dev_num)
{
	return 0;
}

