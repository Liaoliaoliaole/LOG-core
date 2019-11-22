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
#define DATE_LEN 50

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
	char *logstat_path_and_name, *slash, date[DATE_LEN];
	//make time_t variable and get unix time 
	time_t now_time = time(NULL);
	
	logstat_path_and_name = (char *) malloc(sizeof(char) * strlen(logstat_path) + strlen(stats->CAN_IF_name) + strlen("/logstat_12345.json"));
	slash = logstat_path[strlen(logstat_path)-1] == '/' ? "" : "/";
	sprintf(logstat_path_and_name,"%s%slogstat_%s.json",logstat_path, slash, stats->CAN_IF_name);
	//cJSON related variables
	char *JSON_str = NULL;
	cJSON *root = NULL;
    cJSON *list_SDAQs = NULL;
	
	//get and format time
	strftime (date,DATE_LEN,"%x %T",gmtime(&now_time));
    //printf("Version: %s\n", cJSON_Version());
	root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "logstat_build_date(UTC)", cJSON_CreateString(date));
	cJSON_AddNumberToObject(root, "logstat_build_date(UNIX)", now_time);
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

void extract_list_SDAQ_Channels_cal_dates(gpointer node, gpointer arg_pass)
{
	char date[DATE_LEN];
	struct Channel_date_entry *node_dec = (struct Channel_date_entry *)node;
	cJSON *list_SDAQs = (cJSON *)arg_pass;
	cJSON *node_data = cJSON_CreateObject();
	time_t exp_date = node_dec->CH_date.date;
	
	//format time
	strftime(date,DATE_LEN,"%Y/%m",gmtime(&exp_date));
	cJSON_AddNumberToObject(node_data, "Channel", node_dec->Channel);
	cJSON_AddItemToObject(node_data, "Cal_Exp_date", cJSON_CreateString(date));
	cJSON_AddNumberToObject(node_data, "Cal_Exp_date(UNIX)", exp_date);
	cJSON_AddNumberToObject(node_data, "Amount_of_points", node_dec->CH_date.amount_of_points);
	cJSON_AddItemToObject(node_data, "Channel's_Unit", cJSON_CreateString(unit_str[node_dec->CH_date.cal_units]));
	cJSON_AddItemToObject(list_SDAQs, "Calibration_date", node_data);
}

void extract_list_SDAQnode_data(gpointer node, gpointer arg_pass)
{
	char date[DATE_LEN];
	struct SDAQ_info_entry *node_dec = (struct SDAQ_info_entry *)node;
	GSList *SDAQ_Channels_cal_dates = node_dec->SDAQ_Channels_cal_dates;
	cJSON *list_SDAQs = (cJSON *)arg_pass;
	cJSON *list_SDAQ_Channels_cal_dates = cJSON_CreateObject();
	cJSON *node_data = cJSON_CreateObject();
	
	//format time
	strftime(date,DATE_LEN,"%x %T",gmtime(&node_dec->last_seen));
	cJSON_AddNumberToObject(node_data, "Address", node_dec->SDAQ_address);
	cJSON_AddNumberToObject(node_data, "Serial_number", (node_dec->SDAQ_status).dev_sn);
	cJSON_AddItemToObject(node_data, "SDAQ_type", cJSON_CreateString(dev_type_str[(node_dec->SDAQ_info).dev_type]));
	cJSON_AddNumberToObject(node_data, "firm_rev", (node_dec->SDAQ_info).firm_rev);
	cJSON_AddNumberToObject(node_data, "hw_rev", (node_dec->SDAQ_info).hw_rev);
	cJSON_AddNumberToObject(node_data, "Number_of_channels", (node_dec->SDAQ_info).num_of_ch);
	cJSON_AddNumberToObject(node_data, "Sample_rate", (node_dec->SDAQ_info).sample_rate);
	cJSON_AddNumberToObject(node_data, "Max_cal_point", (node_dec->SDAQ_info).max_cal_point);
	cJSON_AddItemToObject(node_data, "Calibration_date",list_SDAQ_Channels_cal_dates = cJSON_CreateArray());
	g_slist_foreach(SDAQ_Channels_cal_dates, extract_list_SDAQ_Channels_cal_dates, list_SDAQ_Channels_cal_dates);
	cJSON_AddStringToObject(node_data, "Last_seen(UTC)", date);
	cJSON_AddNumberToObject(node_data, "Last_seen(UNIX)", node_dec->last_seen);
	cJSON_AddNumberToObject(node_data, "Timediff", node_dec->Timediff);
	cJSON_AddItemToObject(list_SDAQs, "SDAQs_data",node_data);
}

/*
 SDAQ's CAN Device_info message decoder 
typedef struct SDAQ_Info_Decoder{
	unsigned char dev_type;
	unsigned char firm_rev;
	unsigned char hw_rev;
	unsigned char num_of_ch;
	unsigned char sample_rate;
	unsigned char max_cal_point;
}sdaq_info;
*/

