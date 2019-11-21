/*
File "Morfeas_JSON.c" part of Morfeas project, contains implementation of "Morfeas_JSON.h".
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
#include <time.h>

//Header for cJSON
#include <cjson/cJSON.h>
//Include Functions implementation header
#include "Types.h"

//Local functions
void extract_list_SDAQnode_data(gpointer node, gpointer arg_pass);

int logstat_json(char *logstat_path, void *stats_arg)
{
	if(!logstat_path)
		return 1;
	struct Morfeas_SDAQ_if_stats *stats = (struct Morfeas_SDAQ_if_stats *) stats_arg;
	FILE * pFile;
	char *logstat_path_and_name, *slash;
	logstat_path_and_name = (char *) malloc(sizeof(char) * strlen(logstat_path) + strlen(stats->CAN_IF_name));
	slash = logstat_path[strlen(logstat_path)-1] == '/' ? "" : "/";
	sprintf(logstat_path_and_name,"%s%sloagstat_%s.json",logstat_path, slash, stats->CAN_IF_name);
	//cJSON related variables
	char *JSON_str = NULL;
	cJSON *root = NULL;
    cJSON *list_SDAQs = NULL;
    //printf("Version: %s\n", cJSON_Version());
	root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "CANBus-interface", cJSON_CreateString(stats->CAN_IF_name));
	cJSON_AddNumberToObject(root, "BUS_Utilization", stats->Bus_util);
	cJSON_AddNumberToObject(root, "Detected_SDAQs", stats->detected_SDAQs);
	cJSON_AddNumberToObject(root, "Address_Conflicts", stats->conflicts);
	cJSON_AddItemToObject(root, "SDAQs_data",list_SDAQs = cJSON_CreateArray());
	g_slist_foreach(stats->list_SDAQs, extract_list_SDAQnode_data, list_SDAQs);

	JSON_str = cJSON_Print(root);
	//JSON_str = cJSON_PrintUnformatted(root);
	pFile = fopen (logstat_path_and_name, "w");
	if(pFile)
	{
		fputs(JSON_str, pFile);
		fclose (pFile);
	}
	else
		fprintf(stderr,"Write error @ Statlog file\n");
	cJSON_Delete(root);
	free(JSON_str);
	free(logstat_path_and_name);
	return 0;
}

void extract_list_SDAQnode_data(gpointer node, gpointer arg_pass)
{
	struct SDAQ_info_entry *node_dec = (struct SDAQ_info_entry *)node;
	cJSON *list_SDAQs = (cJSON *)arg_pass;
	cJSON *node_data = cJSON_CreateObject();
	cJSON_AddNumberToObject(node_data, "Address", node_dec->SDAQ_address);
	cJSON_AddNumberToObject(node_data, "Serial_number", (node_dec->SDAQ_status).dev_sn);
	cJSON_AddStringToObject(node_data, "Last_seen", ctime (&node_dec->last_seen));
	cJSON_AddNumberToObject(node_data, "Timediff", node_dec->time_diff);
	cJSON_AddItemToObject(list_SDAQs, "SDAQs_data",node_data);
}


/*
// Data of list_SDAQs nodes
struct SDAQ_info_entry{
	unsigned char SDAQ_address;
	short time_diff;
	sdaq_status SDAQ_status;
	sdaq_info SDAQ_info;
	sdaq_calibration_date SDAQ_cal_dates;
	time_t last_seen;
};
*/

