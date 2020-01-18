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

#include "../Morfeas_Types.h"
#include "Morfeas_RPi_Hat.h"

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


