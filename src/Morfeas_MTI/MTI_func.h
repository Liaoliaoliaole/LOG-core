/*
File: MTI_func.c, Declaration of functions for MTI (MODBus), Part of Morfeas_project.
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
#include <modbus.h>

//-- MTI Read Functions --//
int get_MTI_status(modbus_t *ctx, void *arg);//MTI function that request the MTI's status and load them to stats, return 0 on success
int get_MTI_Radio_config(modbus_t *ctx, void *arg);//MTI function that request the MTI's RX configuration. Load configuration status stats and return "telemetry type". 
int get_MTI_Tele_data(modbus_t *ctx, void *arg);//MTI function that request from MTI the telemetry data. Load this data to stats. Return 0 in success

//-- MTI Write Functions --//
int set_MTI_Radio_config(modbus_t *ctx, void *arg);//MTI function that sending a new Radio configuration. Return 0 on success