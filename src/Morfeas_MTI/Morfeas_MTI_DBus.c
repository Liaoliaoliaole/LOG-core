/*
File "Morfeas_MTI_DBus.c" Implementation of D-Bus listener for the MTI handler.
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
#include <math.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <dbus/dbus.h>

#include <modbus.h>

#include "../Morfeas_Types.h"
#include "../Supplementary/Morfeas_Logger.h"

//External Global variables from Morfeas_MTI_if.c
extern volatile unsigned char handler_run;
extern pthread_mutex_t MTI_access;

	//--- MTI's Functions ---//
//MTI function that sending a new Radio configuration. Return 0 on success, errno otherwise.
int set_MTI_Radio_config(modbus_t *ctx, unsigned char new_RF_CH, unsigned char new_mode, union MTI_specific_regs *new_sregs);
//MTI function that set the Global switches. Return 0 on success, errno otherwise.
int set_MTI_Global_switches(modbus_t *ctx, unsigned char global_power, unsigned char global_speed);

//D-Bus listener function
void * MTI_DBus_listener(void *varg_pt)//Thread function.
{
	//Decoded variables from passer
	modbus_t *ctx = *(((struct MTI_DBus_thread_arguments_passer *)varg_pt)->ctx);
	struct Morfeas_MTI_if_stats *stats = ((struct MTI_DBus_thread_arguments_passer *)varg_pt)->stats;
	//Local MTI structs
	union MTI_specific_regs sregs;
	struct Gen_config_struct PWM_gens_config;

	if(!handler_run)//Immediately exit if called with MTI handler under termination
		return NULL;

	Logger("Thread for D-Bus listener Started\n");
	while(handler_run)
	{

		sleep(1);
	}
	Logger("D-Bus listener thread terminated\n");
	return NULL;
}
