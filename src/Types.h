/*
File "Types.h" part of Morfeas project, contains shared Datatypes.
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


//Morfeas_SDAQ-if stats struct
struct Morfeas_SDAQ_if_stats{
	char *CAN_IF_name;
	float Bus_util;
	unsigned char detected_SDAQs;// Amount of online SDAQ.
	unsigned char conflicts;// Amount of SDAQ with conflict addresses
	GSList *list_SDAQs;// List with SDAQ status, info and last seen timestamp.
	GSList *LogBook;//List of the LogBook file
};
// Data of list_SDAQs nodes
struct SDAQ_info_entry{
	unsigned char SDAQ_address;
	short Timediff; 
	sdaq_status SDAQ_status;
	sdaq_info SDAQ_info;
	sdaq_calibration_date SDAQ_cal_dates;
	time_t last_seen;
};

// Data of list_SDAQs nodes
struct LogBook_entry{
	unsigned int SDAQ_sn;
	unsigned char SDAQ_address;
	time_t fisrt_seen;
};