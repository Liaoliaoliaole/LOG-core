/*
File "Morfeas_Types.h" part of Morfeas project, contain the shared Datatypes.
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

#include <gmodule.h>
#include <glib.h>
//Include SDAQ Driver header
#include "sdaq-worker/src/SDAQ_drv.h"

//Array with stings of the Supported Interface_names.
extern char *Morfeas_IPC_handler_type_name[];

//Morfeas_SDAQ-if stats struct
struct Morfeas_SDAQ_if_stats{
	char LogBook_file_path[100];
	int FIFO_fd;
	char *CAN_IF_name;
	float Bus_util;
	float Bus_voltage;
	float Bus_amperage;
	float Shunt_temp;
	unsigned char detected_SDAQs;// Amount of online SDAQ.
	GSList *list_SDAQs;// List with SDAQ status, info and last seen timestamp.
	GSList *LogBook;//List of the LogBook file
};
// Data of a list_SDAQs node
struct SDAQ_info_entry{
	unsigned char SDAQ_address;
	short Timediff;
	sdaq_status SDAQ_status;
	sdaq_info SDAQ_info;
	GSList *SDAQ_Channels_cal_dates;
	time_t last_seen;
	unsigned info_collection_status : 2;//3=all info collected, 2=only Dev_info collected, 1=Dev_info requested 0= nothing has been collected
};
// Data of a SDAQ_cal_dates node
struct Channel_date_entry{
	unsigned char Channel;
	sdaq_calibration_date CH_date;
};
// Data of a list_SDAQs node
struct LogBook_entry{
	unsigned int SDAQ_sn;
	unsigned char SDAQ_address;
}__attribute__((packed, aligned(1)));

//Data of the List ISO_Channels_names
struct ISO_Channel_name{
	char ISO_channel_name_str[20];
};
