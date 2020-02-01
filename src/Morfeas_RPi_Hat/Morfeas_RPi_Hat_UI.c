/*
Program: Morfeas_RPi_Hat_if.c  Implementation of supporting software for Morfeas_RPi_Hat.
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

#include <arpa/inet.h>

#include "Morfeas_RPi_Hat.h"

int main(int argc, char *argv[])
{
	struct Morfeas_RPi_Hat_EEPROM_SDAQnet_Port_config Ports_config[4] = {0};
	struct Morfeas_RPi_Hat_Port_meas Ports_meas[4] = {0};

	/*
	unsigned char *test_ptr = (void *)&test;
	test_ptr[0]=0x12;
	test_ptr[1]=0x34;
	*(unsigned short*)(test_ptr) = htons(*(unsigned short*)(test_ptr));
	printf("0x%04x\n",test.port_current);
	*/
	printf("Not yet Implemented\n");
	/*
	if(!read_port_config(&config[0], 0, 1))
	{
		for(int i=0;i<sizeof(config);i++)
			printf("%c",((unsigned char *)&config[0])[i]);
		putchar('\n');
	}
	*/
	return 0;
}


