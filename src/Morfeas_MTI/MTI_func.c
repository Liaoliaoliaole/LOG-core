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
#define MTI_STATUS_OFFSET 2000

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
	
	if(modbus_read_input_registers(ctx, MTI_STATUS_OFFSET, sizeof(cur_status)/sizeof(short), (unsigned short*)&cur_status)<=0)
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
		float batt_volt;
		float batt_cap;
		float batt_state;
		float CPU_temp;
		float Button_state;
		float PWM_clock;
		float PWM_freq;
		float PWM_Channels[4];
	};
	*/
	printf("MTI_Status CPU_temp = %f\n",cur_status.batt_volt);
	stats->MTI_status.MTI_batt_volt = cur_status.batt_volt;
	stats->MTI_status.MTI_batt_capacity = cur_status.batt_cap;
	stats->MTI_status.MTI_CPU_temp = cur_status.CPU_temp;
	
	return EXIT_SUCCESS;
}