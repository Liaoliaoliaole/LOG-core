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
#include <math.h>
#include <errno.h>

#include <modbus.h>

#include "../Morfeas_Types.h"
#include "../Supplementary/Morfeas_Logger.h"


	//--- MTI's Functions ---//
//MTI function that sending a new Radio configuration. Return 0 on success, errno otherwise. 
int set_MTI_Radio_config(modbus_t *ctx, unsigned char new_RF_CH, unsigned char new_mode, union MTI_specific_regs *new_sregs);