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
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <asm/ioctl.h>

#include <arpa/inet.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>

#include "Morfeas_RPi_Hat.h"
#include "../Morfeas_Types.h"
#include "../Supplementary/Morfeas_run_check.h"

//Decode string CAN_if_name to Port number. Return: Port's Number or -1 on failure
int get_port_num(char * CAN_if_name)
{
	if(!strcmp(CAN_if_name, "can0"))
		return 0;
	else if(!strcmp(CAN_if_name, "can1"))
		return 1;
	else if(!strcmp(CAN_if_name, "can2"))
		return 2;
	else if(!strcmp(CAN_if_name, "can3"))
		return 3;
	else
		return -1;
}
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
		return -1;
	}
	if (1 != write(fd, &s_values_str[!value ? 0 : 1], 1))
	{
		fprintf(stderr, "Failed to write value!\n");
		return -1;
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
		return -1;
	}
	if (read(fd, &read_val, 1) != 1)
	{
		fprintf(stderr, "Failed to write value!\n");
		return -1;
	}
	close(fd);
	return(atoi(read_val));
}
	//---- I2C device related ----//
//Function that write block "data" to I2C device with address "dev_addr" on I2C bus "i2c_dev_num". Return: 0 on success, -1 on failure.
int I2C_write_block(unsigned char i2c_dev_num, unsigned char dev_addr, unsigned char reg, void *data, unsigned char len)
{
	char filename[30];//Path to sysfs I2C-dev
	int i2c_fd;//I2C file descriptor
	int write_bytes;
	unsigned char *data_w_reg;
	//Open I2C-bus
	sprintf(filename, "/dev/i2c-%u", i2c_dev_num);
	i2c_fd = open(filename, O_RDWR);
	if (i2c_fd < 0)
	{
	  perror("Error on I2C open");
	  return -1;
	}
	if (ioctl(i2c_fd, I2C_SLAVE, dev_addr) < 0)
	{
	  perror("Error on ioctl");
	  close(i2c_fd);
	  return -1;
	}
	if(!(data_w_reg = calloc(len+1, sizeof(*data_w_reg))))
	{
	  fprintf(stderr, "Memory error!!!\n");
	  exit(EXIT_FAILURE);
	}
	data_w_reg[0] = reg;
	memcpy(data_w_reg+1, data, len);
	write_bytes = write(i2c_fd, data_w_reg, len);
	free(data_w_reg);
	close(i2c_fd);
	return write_bytes == len ? 0 : -1;
}
//Function that read a block "data" from an I2C device with address "dev_addr" on I2C bus "i2c_dev_num". Return: 0 on success, -1 on failure.
int I2C_read_block(unsigned char i2c_dev_num, unsigned char dev_addr, unsigned char reg, void *data, unsigned char len)
{
	char filename[30];//Path to sysfs I2C-dev
	int ret_val, i2c_fd;//I2C file descriptor
	struct i2c_rdwr_ioctl_data msgset;
	//Open I2C-bus
	sprintf(filename, "/dev/i2c-%u", i2c_dev_num);
	i2c_fd = open(filename, O_RDWR);
	if (i2c_fd < 0)
	{
	  perror("Error on I2C open");
	  return -1;
	}
	//Allocate memory for the messages
	msgset.nmsgs = 2;
	if(!(msgset.msgs = calloc(msgset.nmsgs, sizeof(struct i2c_msg))))
	{
	  fprintf(stderr, "Memory error!!!\n");
	  exit(EXIT_FAILURE);
	}
	//Build message for Write reg
	msgset.msgs[0].addr = dev_addr;
	msgset.msgs[0].flags = 0; //Write
	msgset.msgs[0].len = 1;
	msgset.msgs[0].buf = &reg;
	//Build message for Read *data
	msgset.msgs[1].addr = dev_addr;
	msgset.msgs[1].flags = I2C_M_RD;//Read flag
	msgset.msgs[1].len = len;
	msgset.msgs[1].buf = data;
	//write reg and read the measurements
	if((ret_val = ioctl(i2c_fd, I2C_RDWR, &msgset)) < 0)
		perror("Error @ ioctl");
	close(i2c_fd);
	free(msgset.msgs);
	return ret_val==msgset.nmsgs ? 0 : -1;
}

//Function to init the MAX9611
int MAX9611_init(unsigned char port, unsigned char i2c_dev_num)
{
	/*
	int addr;// Address for MAX9611 connected to port
	switch(port)
	{
		case 0: addr=0x70; break;
		case 1: addr=0x73; break;
		case 2: addr=0x7c; break;
		case 3: addr=0x7f; break;
		default: return -1;
	}
	*/
	return -1;
}
//Function that read measurements for MAX9611, store them on memory pointed by meas.
int get_port_meas(struct Morfeas_RPi_Hat_Port_meas *meas, unsigned char port, unsigned char i2c_dev_num)
{
	int ret_val, addr;// Address for MAX9611 connected to port
	unsigned short *meas_dec = (unsigned short *)meas;
	switch(port)
	{
		case 0: addr=0x70; break;
		case 1: addr=0x73; break;
		case 2: addr=0x7c; break;
		case 3: addr=0x7f; break;
		default: return -1;
	}
	ret_val = I2C_read_block(i2c_dev_num, addr, 0, meas, sizeof(struct Morfeas_RPi_Hat_Port_meas));
	for(int i=0; i<sizeof(struct Morfeas_RPi_Hat_Port_meas); i++)
	{
		*(meas_dec+i) = htons(*(meas_dec+i));
		*(meas_dec+i) >>= 4;
	}
	return ret_val;
}
//Function that read data from EEPROM(24AA08)
int read_port_config(struct Morfeas_RPi_Hat_EEPROM_SDAQnet_Port_config *config, unsigned char port, unsigned char i2c_dev_num)
{
	struct Morfeas_RPi_Hat_EEPROM_SDAQnet_Port_config config_read;
	unsigned int blank_check=0;
	int addr;// Address for 24AA08 EEPROM
	unsigned char checksum;
	switch(port)
	{
		case 0: addr=0x50; break;
		case 1: addr=0x51; break;
		case 2: addr=0x52; break;
		case 3: addr=0x53; break;
		default: return -1;
	}
	//Get data from EEPROM
	if(I2C_read_block(i2c_dev_num, addr, 0, &config_read, sizeof(struct Morfeas_RPi_Hat_EEPROM_SDAQnet_Port_config)))
	{
		fprintf(stderr, "Configuration EEPROM(24AA08) Not found!!!\n");
		return -1;
	}
	//Check if EEPROM is blank
	for(int i=0; i<sizeof(struct Morfeas_RPi_Hat_EEPROM_SDAQnet_Port_config); i++)
	{
		if(((char*)&config_read)[i]==-1)
			blank_check++;
	}
	if(blank_check == sizeof(struct Morfeas_RPi_Hat_EEPROM_SDAQnet_Port_config))
	{
		fprintf(stderr, "EEPROM is Blank!!!\n");
		return 2;
	}
	//Calculate and compare Checksum
	checksum = Checksum(&config_read, sizeof(struct Morfeas_RPi_Hat_EEPROM_SDAQnet_Port_config)-1);
	if(config_read.checksum ^ checksum)
	{
		fprintf(stderr, "Checksum Error!!!\n");
		return 1;
	}
	memcpy(config, &config_read, sizeof(struct Morfeas_RPi_Hat_EEPROM_SDAQnet_Port_config));
	return 0;
}
//Function that write data to EEPROM(24AA08). Checksum calculated inside.
int write_port_config(struct Morfeas_RPi_Hat_EEPROM_SDAQnet_Port_config *config, unsigned char port, unsigned char i2c_dev_num)
{
	unsigned char zero=0;
	struct i2c_rdwr_ioctl_data msgset;
	struct Morfeas_RPi_Hat_EEPROM_SDAQnet_Port_config read_config;
	char filename[30];//Path to sysfs I2C-dev
	int addr;// Address for 24AA08 EEPROM
	int i2c_fd;//I2C file descriptor
	unsigned char data_w_reg[17];//16 data bytes + 1 register byte
	int len, reg=0;

	//Calc addr for port argument
	switch(port)
	{
		case 0: addr=0x50; break;
		case 1: addr=0x51; break;
		case 2: addr=0x52; break;
		case 3: addr=0x53; break;
		default: return -1;
	}
	//Open I2C-bus
	sprintf(filename, "/dev/i2c-%u", i2c_dev_num);
	i2c_fd = open(filename, O_RDWR);
	if (i2c_fd < 0)
	{
	  perror("Error on I2C open");
	  return 3;
	}
	//Set addr as I2C_SLAVE address
	if (ioctl(i2c_fd, I2C_SLAVE, addr) < 0)
	{
	  perror("Error on ioctl");
	  close(i2c_fd);
	  return 4;
	}
	//Check device existence.
	if(i2c_smbus_write_quick(i2c_fd, 0))
	{
		fprintf(stderr, "Configuration EEPROM(24AA08) Not found!!!\n");
		close(i2c_fd);
		return -1;
	}
	//Calculate checksum
	config->checksum = Checksum(config, sizeof(struct Morfeas_RPi_Hat_EEPROM_SDAQnet_Port_config)-1);
	//Write data blocks to EEPROM. TO-DO Check
	while(reg<sizeof(struct Morfeas_RPi_Hat_EEPROM_SDAQnet_Port_config))
	{
		data_w_reg[0] = reg;
		len = sizeof(struct Morfeas_RPi_Hat_EEPROM_SDAQnet_Port_config) - reg;
		memcpy(data_w_reg+1, ((void *)config)+reg, len);
		if(write(i2c_fd, data_w_reg, len) != len)
		{
		  fprintf(stderr, "Error I2C_Write!!!\n");
		  close(i2c_fd);
		  return -1;
		}
		usleep(10000);//Sleep for 10msec
		reg += len;
	}
	//Read data block from EEPROM.
	//Allocate memory for the messages
	msgset.nmsgs = 2;
	if(!(msgset.msgs = calloc(msgset.nmsgs, sizeof(struct i2c_msg))))
	{
	  fprintf(stderr, "Memory error!!!\n");
	  exit(EXIT_FAILURE);
	}
	//Build message for Write reg at 0
	msgset.msgs[0].addr = addr;
	msgset.msgs[0].flags = 0; //Write
	msgset.msgs[0].len = 1;
	msgset.msgs[0].buf = &zero;
	//Build message for Read *data
	msgset.msgs[1].addr = addr;
	msgset.msgs[1].flags = I2C_M_RD;//Read flag
	msgset.msgs[1].len = len;
	msgset.msgs[1].buf = (void *)&read_config;
	//write reg and read the measurements
	if(ioctl(i2c_fd, I2C_RDWR, &msgset) < 0)
	{
		perror("Error @ ioctl");
		close(i2c_fd);
		free(msgset.msgs);
		return -1;
	}
	close(i2c_fd);
	free(msgset.msgs);
	//Verification
	for(int i=0; i<sizeof(struct Morfeas_RPi_Hat_EEPROM_SDAQnet_Port_config); i++)
	{
		if(((unsigned char*)&read_config)[i]!=((unsigned char*)config)[i])
		{
			fprintf(stderr, "Write_port_config: Verification Failed!!!\n");
			return -1;
		}
	}
	return 0;
}

