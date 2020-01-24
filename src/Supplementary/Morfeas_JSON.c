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
#define STR_LEN 50

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include <arpa/inet.h>

#include <modbus.h>

//Header for cJSON
#include <cjson/cJSON.h>
//Include Functions implementation header
#include "../Morfeas_Types.h"

//Local functions
void extract_list_SDAQnode_data(gpointer node, gpointer arg_pass);

//Delete logstat file for IOBOX_handler
int delete_logstat_SDAQ(char *logstat_path, void *stats_arg)
{
	if(!logstat_path || !stats_arg)
		return -1;
	int ret_val;
	char *logstat_path_and_name, *slash;
	struct Morfeas_SDAQ_if_stats *stats = stats_arg;

	logstat_path_and_name = (char *) malloc(sizeof(char) * strlen(logstat_path) + strlen(stats->CAN_IF_name) + strlen("/logstat_12345.json") + 1);
	slash = logstat_path[strlen(logstat_path)-1] == '/' ? "" : "/";
	sprintf(logstat_path_and_name,"%s%slogstat_%s.json",logstat_path, slash, stats->CAN_IF_name);
	//Delete logstat file
	ret_val = unlink(logstat_path_and_name);
	free(logstat_path_and_name);
	return ret_val;
}

int logstat_SDAQ(char *logstat_path, void *stats_arg)
{
	if(!logstat_path || !stats_arg)
		return -1;
	struct Morfeas_SDAQ_if_stats *stats = stats_arg;
	FILE * pFile;
	static unsigned char write_error = 0;
	char *logstat_path_and_name, *slash, date[STR_LEN];
	//make time_t variable and get unix time
	time_t now_time = time(NULL);

	logstat_path_and_name = (char *) malloc(sizeof(char) * strlen(logstat_path) + strlen(stats->CAN_IF_name) + strlen("/logstat_12345.json") + 1);
	slash = logstat_path[strlen(logstat_path)-1] == '/' ? "" : "/";
	sprintf(logstat_path_and_name,"%s%slogstat_%s.json",logstat_path, slash, stats->CAN_IF_name);
	//cJSON related variables
	char *JSON_str = NULL;
	cJSON *root = NULL;
    cJSON *logstat = NULL;

	//get and format time
	strftime (date,STR_LEN,"%x %T",gmtime(&now_time));
    //printf("Version: %s\n", cJSON_Version());
	root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "logstat_build_date_UTC", cJSON_CreateString(date));
	cJSON_AddNumberToObject(root, "logstat_build_date_UNIX", now_time);
	cJSON_AddItemToObject(root, "CANBus-interface", cJSON_CreateString(stats->CAN_IF_name));
	cJSON_AddNumberToObject(root, "BUS_voltage", roundf(100.0 * stats->Bus_voltage)/100.0);
	cJSON_AddNumberToObject(root, "BUS_amperage", roundf(1000.0 * stats->Bus_amperage)/1000.0);
	cJSON_AddNumberToObject(root, "BUS_Shunt_Res_temp", stats->Shunt_temp);
	cJSON_AddNumberToObject(root, "BUS_Utilization", roundf(100.0 * stats->Bus_util)/100.0);
	cJSON_AddNumberToObject(root, "Detected_SDAQs", stats->detected_SDAQs);
	cJSON_AddItemToObject(root, "SDAQs_data",logstat = cJSON_CreateArray());
	g_slist_foreach(stats->list_SDAQs, extract_list_SDAQnode_data, logstat);

	//JSON_str = cJSON_Print(root);
	JSON_str = cJSON_PrintUnformatted(root);
	pFile = fopen (logstat_path_and_name, "w");
	if(pFile)
	{
		fputs(JSON_str, pFile);
		fclose (pFile);
		if(write_error)
			fprintf(stderr,"Write error @ Statlog file, Restored\n");
		write_error = 0;
	}
	else if(!write_error)
	{
		fprintf(stderr,"Write error @ Statlog file!!!\n");
		write_error = -1;
	}
	cJSON_Delete(root);
	free(JSON_str);
	free(logstat_path_and_name);
	return 0;
}

void extract_list_SDAQ_Channels_cal_dates(gpointer node, gpointer arg_pass)
{
	char date[STR_LEN];
	struct Channel_date_entry *node_dec = node;
	struct tm cal_date = {0};
	cJSON *array = arg_pass;
	cJSON *node_data;
	if(node)
	{
		node_data = cJSON_CreateObject();
		cal_date.tm_year = node_dec->CH_date.year + 100;//100 = 2000 - 1900
		cal_date.tm_mon = node_dec->CH_date.month - 1;
		cal_date.tm_mday = node_dec->CH_date.day;
		//format time
		strftime(date,STR_LEN,"%Y/%m/%d",&cal_date);
		cJSON_AddNumberToObject(node_data, "Channel", node_dec->Channel);
		cJSON_AddItemToObject(node_data, "Calibration_date", cJSON_CreateString(date));
		cJSON_AddNumberToObject(node_data, "Calibration_date_UNIX", mktime(&cal_date));
		cJSON_AddNumberToObject(node_data, "Calibration_period", node_dec->CH_date.period);
		cJSON_AddNumberToObject(node_data, "Amount_of_points", node_dec->CH_date.amount_of_points);
		cJSON_AddItemToObject(node_data, "Unit", cJSON_CreateString(unit_str[node_dec->CH_date.cal_units]));
		cJSON_AddItemToObject(node_data, "Is_calibrated", cJSON_CreateBool(node_dec->CH_date.cal_units >= Unit_code_base_region_size));
		cJSON_AddNumberToObject(node_data, "Unit_code", node_dec->CH_date.cal_units);
		cJSON_AddItemToArray(array, node_data);
	}
}

void extract_list_SDAQ_Channels_acc_to_avg_meas(gpointer node, gpointer arg_pass)
{
	struct Channel_acc_meas_entry *node_dec = node;
	cJSON *array = arg_pass;
	cJSON *node_data, *channel_status;
	if(node)
	{
		node_data = cJSON_CreateObject();
		cJSON_AddNumberToObject(node_data, "Channel", node_dec->Channel);
		//-- Add SDAQ's Channel Status --//
		cJSON_AddItemToObject(node_data, "Channel_Status", channel_status = cJSON_CreateObject());
		cJSON_AddNumberToObject(channel_status, "Channel_status_val", node_dec->status);
		cJSON_AddItemToObject(channel_status, "Out_of_Range", cJSON_CreateBool(node_dec->status & (1<<Out_of_range)));
		cJSON_AddItemToObject(channel_status, "No_Sensor", cJSON_CreateBool(node_dec->status & (1<<No_sensor)));
		//-- Add Unit of Channel --//
		cJSON_AddItemToObject(node_data, "Unit", cJSON_CreateString(unit_str[node_dec->unit_code]));
		//-- Add Averaged measurement of Channel --//
		if(!(node_dec->status & (1<<No_sensor)))
		{
			node_dec->meas_acc/=(float)node_dec->cnt;
			cJSON_AddNumberToObject(node_data, "Meas_avg", roundf(1000.0 * node_dec->meas_acc)/1000.0);
			node_dec->meas_acc = 0;
			node_dec->cnt = 0;
		}
		else
			cJSON_AddNumberToObject(node_data, "Meas_avg", NAN);
		cJSON_AddItemToObject(array, "Measurement_data", node_data);
	}
}

void extract_list_SDAQnode_data(gpointer node, gpointer arg_pass)
{
	char date[STR_LEN];
	struct SDAQ_info_entry *node_dec = (struct SDAQ_info_entry *)node;
	GSList *SDAQ_Channels_cal_dates = node_dec->SDAQ_Channels_cal_dates;
	GSList *SDAQ_Channels_acc_meas = node_dec->SDAQ_Channels_acc_meas;
	cJSON *list_SDAQs_array, *node_data, *list_SDAQ_Channels_cal_dates, *SDAQ_status, *SDAQ_info, *list_SDAQ_Channels_acc_meas;
	if(node)
	{
		list_SDAQs_array = arg_pass;
		node_data = cJSON_CreateObject();
		//-- Add SDAQ's general data to SDAQ's JSON object --//
		strftime(date,STR_LEN,"%x %T",gmtime(&node_dec->last_seen));//format time
		cJSON_AddStringToObject(node_data, "Last_seen_UTC", date);
		cJSON_AddNumberToObject(node_data, "Last_seen_UNIX", node_dec->last_seen);
		cJSON_AddNumberToObject(node_data, "Address", node_dec->SDAQ_address);
		cJSON_AddNumberToObject(node_data, "Serial_number", (node_dec->SDAQ_status).dev_sn);
		cJSON_AddItemToObject(node_data, "SDAQ_type", cJSON_CreateString(dev_type_str[(node_dec->SDAQ_info).dev_type]));
		cJSON_AddNumberToObject(node_data, "Timediff", node_dec->Timediff);
		//-- Add SDAQ's Status --//
		cJSON_AddItemToObject(node_data, "SDAQ_Status", SDAQ_status = cJSON_CreateObject());
		cJSON_AddNumberToObject(SDAQ_status, "SDAQ_status_val", (node_dec->SDAQ_status).status);
		cJSON_AddItemToObject(SDAQ_status, "In_sync", cJSON_CreateBool((node_dec->SDAQ_status).status & (1<<In_sync)));
		cJSON_AddItemToObject(SDAQ_status, "Error", cJSON_CreateBool((node_dec->SDAQ_status).status & (1<<Error)));
		cJSON_AddItemToObject(SDAQ_status, "State", cJSON_CreateString(status_byte_dec((node_dec->SDAQ_status).status, State)));
		cJSON_AddItemToObject(SDAQ_status, "Mode", cJSON_CreateString(status_byte_dec((node_dec->SDAQ_status).status, Mode)));
		//-- Add SDAQ's Info --//
		cJSON_AddItemToObject(node_data, "SDAQ_info", SDAQ_info = cJSON_CreateObject());
		cJSON_AddNumberToObject(SDAQ_info, "firm_rev", (node_dec->SDAQ_info).firm_rev);
		cJSON_AddNumberToObject(SDAQ_info, "hw_rev", (node_dec->SDAQ_info).hw_rev);
		cJSON_AddNumberToObject(SDAQ_info, "Number_of_channels", (node_dec->SDAQ_info).num_of_ch);
		cJSON_AddNumberToObject(SDAQ_info, "Sample_rate", (node_dec->SDAQ_info).sample_rate);
		cJSON_AddNumberToObject(SDAQ_info, "Max_cal_point", (node_dec->SDAQ_info).max_cal_point);
		//-- Add SDAQ's channel Cal dates  --//
		cJSON_AddItemToObject(node_data, "Calibration_Data",list_SDAQ_Channels_cal_dates = cJSON_CreateArray());
		g_slist_foreach(SDAQ_Channels_cal_dates, extract_list_SDAQ_Channels_cal_dates, list_SDAQ_Channels_cal_dates);
		//-- Add SDAQ's channel average measurement  --//
		cJSON_AddItemToObject(node_data, "Meas",list_SDAQ_Channels_acc_meas = cJSON_CreateArray());
		g_slist_foreach(SDAQ_Channels_acc_meas, extract_list_SDAQ_Channels_acc_to_avg_meas, list_SDAQ_Channels_acc_meas);
		//-- Add SDAQ's Data to Array --//
		cJSON_AddItemToArray(list_SDAQs_array, node_data);
	}
}

//Delete logstat file for IOBOX_handler
int delete_logstat_IOBOX(char *logstat_path, void *stats_arg)
{
	int ret_val;
	if(!logstat_path || !stats_arg)
		return -1;
	char *logstat_path_and_name, *slash;
	struct Morfeas_IOBOX_if_stats *stats = stats_arg;

	logstat_path_and_name = (char *) malloc(sizeof(char) * strlen(logstat_path) + strlen(stats->dev_name) + strlen("/logstat_IOBOX_12345.json") + 1);
	slash = logstat_path[strlen(logstat_path)-1] == '/' ? "" : "/";
	sprintf(logstat_path_and_name,"%s%slogstat_IOBOX_%s.json",logstat_path, slash, stats->dev_name);
	//Delete logstat file
	ret_val = unlink(logstat_path_and_name);
	free(logstat_path_and_name);
	return ret_val;
}
//Converting and exporting function for IOBOX Modbus register. Convert it to JSON format and save it to logstat_path
int logstat_IOBOX(char *logstat_path, void *stats_arg)
{
	if(!logstat_path || !stats_arg)
		return -1;
	struct Morfeas_IOBOX_if_stats *stats = stats_arg;
	unsigned int Identifier;
	FILE * pFile;
	static unsigned char write_error = 0;
	char *logstat_path_and_name, *slash, date[STR_LEN];
	char str_buff[10] = {0};
	float value;
	//make time_t variable and get unix time
	time_t now_time = time(NULL);
	//Correct logstat_path_and_name
	logstat_path_and_name = (char *) malloc(sizeof(char) * strlen(logstat_path) + strlen(stats->dev_name) + strlen("/logstat_IOBOX_12345.json") + 1);
	slash = logstat_path[strlen(logstat_path)-1] == '/' ? "" : "/";
	sprintf(logstat_path_and_name,"%s%slogstat_IOBOX_%s.json",logstat_path, slash, stats->dev_name);
	//cJSON related variables
	char *JSON_str = NULL;
	cJSON *root = NULL;
    cJSON *pow_supp_data = NULL;
	cJSON *RX_json = NULL;
	
	//get and format time
	strftime (date,STR_LEN,"%x %T",gmtime(&now_time));
	//Convert IPv4 to IOBOX's Identifier
	inet_pton(AF_INET, stats->IOBOX_IPv4_addr, &Identifier);
	root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "logstat_build_date_UTC", cJSON_CreateString(date));
	cJSON_AddNumberToObject(root, "logstat_build_date_UNIX", now_time);
	cJSON_AddItemToObject(root, "Dev_name", cJSON_CreateString(stats->dev_name));
	cJSON_AddItemToObject(root, "IPv4_address", cJSON_CreateString(stats->IOBOX_IPv4_addr));
	cJSON_AddNumberToObject(root, "Identifier", Identifier);
	if(!stats->error)
	{
		//Add connection status string item in JSON
		cJSON_AddItemToObject(root, "Connection_status", cJSON_CreateString("Okay"));
		//Add Wireless_Inductive_Power_Supply data
		cJSON_AddItemToObject(root, "Power_Supply",pow_supp_data = cJSON_CreateObject());
		cJSON_AddNumberToObject(pow_supp_data, "Vin", roundf(100.0 * stats->Supply_Vin/stats->counter)/100.0);
		stats->Supply_Vin = 0;
		for(int i=0; i<4; i++)
		{
			sprintf(str_buff, "CH%1u_Vout", i+1);
			cJSON_AddNumberToObject(pow_supp_data, str_buff, roundf(100.0 * stats->Supply_meas[i].Vout/stats->counter)/100.0);
			stats->Supply_meas[i].Vout = 0;
			sprintf(str_buff, "CH%1u_Iout", i+1);
			cJSON_AddNumberToObject(pow_supp_data, str_buff, roundf(100.0 * stats->Supply_meas[i].Iout/stats->counter)/100.0);
			stats->Supply_meas[i].Iout = 0;
		}
		//Add RX_Data
		for(int i=0; i<4; i++)
		{
			sprintf(str_buff, "RX%1u", i+1);
			if(stats->RX[i].status && stats->RX[i].success)
			{
				cJSON_AddItemToObject(root, str_buff, RX_json = cJSON_CreateObject());
				for(int j=0; j<16; j++)
				{
						
					sprintf(str_buff, "CH%1u", j+1);
					value = stats->RX[i].CH_value[j]/stats->counter;
					stats->RX[i].CH_value[j] = 0;
					if(value<1500.0)
						cJSON_AddNumberToObject(RX_json, str_buff, roundf(100.0 * value)/100.0);
					else
						cJSON_AddItemToObject(RX_json, str_buff, cJSON_CreateString("No sensor"));
				
				}
				cJSON_AddNumberToObject(RX_json, "Index", stats->RX[i].index);
				cJSON_AddNumberToObject(RX_json, "Status", stats->RX[i].status);
				cJSON_AddNumberToObject(RX_json, "Success", stats->RX[i].success);
			}
			else
				cJSON_AddItemToObject(root, str_buff, cJSON_CreateString("Disconnected"));
		}
	}
	else
		cJSON_AddItemToObject(root, "Connection_status", cJSON_CreateString(modbus_strerror(stats->error)));
	//Reset accumulator counter
	stats->counter = 0;
	//Print JSON to File
	//JSON_str = cJSON_Print(root);
	JSON_str = cJSON_PrintUnformatted(root);
	pFile = fopen (logstat_path_and_name, "w");
	if(pFile)
	{
		fputs(JSON_str, pFile);
		fclose (pFile);
		if(write_error)
			fprintf(stderr,"Write error @ Statlog file, Restored\n");
		write_error = 0;
	}
	else if(!write_error)
	{
		fprintf(stderr,"Write error @ Statlog file!!!\n");
		write_error = -1;
	}
	cJSON_Delete(root);
	free(JSON_str);
	free(logstat_path_and_name);
	return 0;
}

//Delete logstat file for IOBOX_handler
int delete_logstat_MDAQ(char *logstat_path, void *stats_arg)
{
	int ret_val;
	if(!logstat_path || !stats_arg)
		return -1;
	char *logstat_path_and_name, *slash;
	struct Morfeas_MDAQ_if_stats *stats = stats_arg;

	logstat_path_and_name = (char *) malloc(sizeof(char) * strlen(logstat_path) + strlen(stats->dev_name) + strlen("/logstat_MDAQ_12345.json") + 1);
	slash = logstat_path[strlen(logstat_path)-1] == '/' ? "" : "/";
	sprintf(logstat_path_and_name,"%s%slogstat_MDAQ_%s.json",logstat_path, slash, stats->dev_name);
	//Delete logstat file
	ret_val = unlink(logstat_path_and_name);
	free(logstat_path_and_name);
	return ret_val;
}

//Converting and exporting function for MDAQ Modbus register. Convert it to JSON format and save it to logstat_path
int logstat_MDAQ(char *logstat_path, void *stats_arg)
{
	if(!logstat_path || !stats_arg)
		return -1;
	struct Morfeas_MDAQ_if_stats *stats = stats_arg;
	unsigned int Identifier;
	FILE * pFile;
	static unsigned char write_error = 0;
	char *logstat_path_and_name, *slash, date[STR_LEN];
	char str_buff[30] = {0};
	//make time_t variable and get unix time
	time_t now_time = time(NULL);
	//Correct logstat_path_and_name
	logstat_path_and_name = (char *) malloc(sizeof(char) * strlen(logstat_path) + strlen(stats->dev_name) + strlen("/logstat_MDAQ_12345.json") + 1);
	slash = logstat_path[strlen(logstat_path)-1] == '/' ? "" : "/";
	sprintf(logstat_path_and_name,"%s%slogstat_MDAQ_%s.json",logstat_path, slash, stats->dev_name);
	//cJSON related variables
	char *JSON_str = NULL;
	cJSON *root = NULL, *Channels_array = NULL, *Channel = NULL, *values = NULL, *warnings = NULL;
	
	//get and format time
	strftime (date,STR_LEN,"%x %T",gmtime(&now_time));
	//Convert IPv4 to MDAQ's Identifier
	inet_pton(AF_INET, stats->MDAQ_IPv4_addr, &Identifier);
	root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "logstat_build_date_UTC", cJSON_CreateString(date));
	cJSON_AddNumberToObject(root, "logstat_build_date_UNIX", now_time);
	cJSON_AddItemToObject(root, "Dev_name", cJSON_CreateString(stats->dev_name));
	cJSON_AddItemToObject(root, "IPv4_address", cJSON_CreateString(stats->MDAQ_IPv4_addr));
	cJSON_AddNumberToObject(root, "Identifier", Identifier);
	if(!stats->error)
	{
		cJSON_AddItemToObject(root, "Connection_status", cJSON_CreateString("Okay"));
		//Add MDAQ Board data
		cJSON_AddNumberToObject(root, "Index", roundf(stats->meas_index));
		cJSON_AddNumberToObject(root, "Board_temp", roundf(100.0 * stats->board_temp/stats->counter)/100.0);
		stats->board_temp = 0;
		//Add array with channels measurements on JSON
		cJSON_AddItemToObject(root, "MDAQ_Channels",Channels_array = cJSON_CreateArray());
		//Load MDAQ Channels Data to stats
		for(int i=0; i<8; i++)
		{
			Channel = cJSON_CreateObject();
			cJSON_AddItemToObject(Channel, "Values", values = cJSON_CreateObject());
			for(int j=0; j<4; j++)
			{
				cJSON_AddNumberToObject(Channel, "Channel", i+1);
				sprintf(str_buff, "Value%1hhu", j+1);
				if(!((stats->meas[i].warnings>>j)&1))
					cJSON_AddNumberToObject(values, str_buff, roundf(100.0 * stats->meas[i].value[j]/stats->counter)/100.0);
				else
					cJSON_AddNumberToObject(values, str_buff, NAN);
				stats->meas[i].value[j] = 0;
			}
			cJSON_AddItemToObject(Channel, "Warnings", warnings = cJSON_CreateObject());
			cJSON_AddNumberToObject(warnings, "Warnings_value", stats->meas[i].warnings);
			for(int j=0; j<4; j++)
			{
				sprintf(str_buff, "Is_Value%1hhu_valid", j+1);
				cJSON_AddItemToObject(warnings, str_buff, cJSON_CreateBool(!((stats->meas[i].warnings>>j)&1)));
			}
			cJSON_AddItemToArray(Channels_array, Channel);
		}
	}
	else
		cJSON_AddItemToObject(root, "Connection_status", cJSON_CreateString(modbus_strerror(stats->error)));
	//Reset accumulator counter
	stats->counter = 0;
	//Print JSON to File
	//JSON_str = cJSON_Print(root);
	JSON_str = cJSON_PrintUnformatted(root);
	pFile = fopen (logstat_path_and_name, "w");
	if(pFile)
	{
		fputs(JSON_str, pFile);
		fclose (pFile);
		if(write_error)
			fprintf(stderr,"Write error @ Statlog file, Restored\n");
		write_error = 0;
	}
	else if(!write_error)
	{
		fprintf(stderr,"Write error @ Statlog file!!!\n");
		write_error = -1;
	}
	cJSON_Delete(root);
	free(JSON_str);
	free(logstat_path_and_name);
	return 0;
}
