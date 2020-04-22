/*
File: MTI_func.c, Implementation of functions for MTI (MODBus), Part of Morfeas_project.
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
#define MTI_STATUS_OFFSET 2001

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <arpa/inet.h>

#include <modbus.h>

#include "../Morfeas_Types.h" 

int get_MTI_status(modbus_t *ctx, struct Morfeas_MTI_if_stats *stats)
{
	struct MTI_dev_status cur_status;
	
	if(modbus_read_input_registers(ctx, MTI_STATUS_OFFSET, sizeof(cur_status)/sizeof(short), (short unsigned int *)&cur_status)<=0)
		return EXIT_FAILURE;
	/*
	struct MTI_status_struct{
		float MTI_batt_volt;
		float MTI_batt_capacity;
		char *MTI_charge_status_str;
		float MTI_CPU_temp;
		struct{
			unsigned char pb1:1;
			unsigned char pb2:1;
			unsigned char pb3:1;
		}buttons_state;
	};
	
	struct MTI_dev_status{
	unsigned short batt_volt;
	unsigned short batt_cap;
	unsigned short batt_state;
	unsigned short CPU_temp;
	unsigned short Button_state;
	unsigned short PWM_clock;
	unsigned short PWM_freq;
	unsigned short PWM_Channels[4];
	};
	*/
	stats->MTI_status.MTI_batt_volt = cur_status.batt_volt;
	stats->MTI_status.MTI_batt_capacity = cur_status.batt_cap;
	stats->MTI_status.MTI_CPU_temp = cur_status.CPU_temp;
	
	return EXIT_SUCCESS;
}