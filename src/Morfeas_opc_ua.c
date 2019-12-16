/*
Program: Morfeas-opc_ua. OPC-UA server software of the Morfeas_core project.
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
#define VERSION "0.9.5 beta" /*Release Version of Morfeas_opc_ua*/
#define n_threads 1
#define CPU_temp_sysfs_file "/sys/class/thermal/thermal_zone0/temp"

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

#include <glib.h>
#include <glibtop.h>
#include <glibtop/uptime.h>
#include <glibtop/cpu.h>
#include <glibtop/mem.h>
#include <glibtop/fsusage.h>

//Include Functions implementation header
#include "Morfeas_handlers_nodeset.h"

//print the Usage manual
void print_usage(char *prog_name);
//FIFO reader, Thread function.
void* IPC_Receiver(void *varg_pt);
//Timer Handler Function
void Rpi_health_update(int sign);
//OPC_UA local Functions
void Morfeas_opc_ua_root_nodeset_Define(UA_Server *server);

//Global variables
static volatile UA_Boolean running = true;
UA_Server *server;

static void stopHandler(int sign)
{
    if(sign==SIGINT)
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
	running = false;
}

int main(int argc, char *argv[])
{
	char *app_name=NULL, *ns_config=NULL;
	//Open62541 OPC-UA variables
	UA_ServerConfig conf = {0};
	UA_StatusCode retval;
	UA_UInt16 timeout;
	//variables for threads
	pthread_t Threads_ids[n_threads];
	int c;
	//Get options 
	while ((c = getopt (argc, argv, "hVc:a:")) != -1)
	{
		switch (c)
		{
			case 'h'://help
				print_usage(argv[0]);
				exit(EXIT_SUCCESS);
			case 'V'://Version
				printf(VERSION"\n");
				exit(EXIT_SUCCESS);
			case 'c'://nodeset config XML file
				ns_config = optarg;
				break;
			case 'a'://OPC-UA application name
				app_name = optarg;
				break;
			case '?':
				print_usage(argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	//Install stopHandler as the signal handler for SIGINT and SIGTERM signals.
	signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

	//Write to Log a welcome message
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
	"\t------ Morfeas OPC-UA Started ------");
	//Setup config for Morfeas_OPC_UA Server
	retval = Morfeas_OPC_UA_config(&conf, app_name, VERSION);
	//Init OPC_UA Server
	server = UA_Server_newWithConfig(&conf);
	//Add Morfeas application base node set to server
	Morfeas_opc_ua_root_nodeset_Define(server);

	//----Start threads----//
	pthread_create(&Threads_ids[0], NULL, IPC_Receiver, NULL);

	//Start OPC_UA Server
	retval = UA_Server_run_startup(server);
    if(retval == UA_STATUSCODE_GOOD)
	{
		while(running)
		{
			pthread_mutex_lock(&OPC_UA_NODESET_access);
			 	timeout = UA_Server_run_iterate(server, false);
			pthread_mutex_unlock(&OPC_UA_NODESET_access);
			usleep(timeout * 100);
		}
		retval = UA_Server_run_shutdown(server);
	}
	//Wait until all threads ends
	for(int i=0; i<n_threads; i++)
		pthread_join(Threads_ids[i], NULL);// wait for threads to finish
    UA_Server_delete(server);
	unlink("/tmp/.Morfeas_FIFO");
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

//print the Usage manual
void print_usage(char *prog_name)
{
	const char preamp[] = {
	"\tProgram: Morfeas_opc_ua  Copyright (C) 12019-12020  Sam Harry Tzavaras\n"
    "\tThis program comes with ABSOLUTELY NO WARRANTY; for details see LICENSE.\n"
    "\tThis is free software, and you are welcome to redistribute it\n"
    "\tunder certain conditions; for details see LICENSE.\n"
	};
	const char manual[] = {
		"Options:\n"
		"           -h : Print help.\n"
		"           -V : Version.\n"
		"           -c : Path to Nodeset configuration XML file.\n"
		"           -a : OPC-UA Application Name.\n"
		"\n"
	};
	printf("%s\nUsage: %s [Options]\n\n%s",preamp, prog_name, manual);
	return;
}

//IPC_Receiver, Thread function.
void* IPC_Receiver(void *varg_pt)
{
	UA_DateTime cal_time;
	UA_DateTimeStruct calibration_date = {0};
	//Morfeas IPC msg decoder
	IPC_message IPC_msg_dec;
	time_t health_update_check=0;
	unsigned char type;//type of received IPC_msg
	char Node_ID_str[60], meas_status_str[60];
	int FIFO_fd;
	//Make the Named Pipe(FIFO)
	mkfifo(Data_FIFO, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    FIFO_fd = open(Data_FIFO, O_RDWR );//O_NONBLOCK | O_RSYNC, O_RDONLY
	while (running)
	{
		if((type = IPC_msg_RX(FIFO_fd, &IPC_msg_dec)))
		{
			//printf("\t--- Received IPC msg of type %d ---\n",type);
			switch(type)
			{
				//Msg type from SDAQ_handler
				case IPC_SDAQ_meas:
					pthread_mutex_lock(&OPC_UA_NODESET_access);
						sprintf(Node_ID_str, "%s.%d.CH%hhu.unit",IPC_msg_dec.SDAQ_meas.connected_to_BUS,
															     IPC_msg_dec.SDAQ_meas.SDAQ_serial_number,
																 IPC_msg_dec.SDAQ_meas.channel);
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str),
														   unit_str[IPC_msg_dec.SDAQ_meas.SDAQ_channel_meas.unit],
														   UA_TYPES_STRING);
						sprintf(Node_ID_str, "%s.%d.CH%hhu.timestamp",IPC_msg_dec.SDAQ_meas.connected_to_BUS,
															     IPC_msg_dec.SDAQ_meas.SDAQ_serial_number,
																 IPC_msg_dec.SDAQ_meas.channel);
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str),
														   &(IPC_msg_dec.SDAQ_meas.SDAQ_channel_meas.timestamp),
														   UA_TYPES_UINT16);
						sprintf(Node_ID_str, "%s.%d.CH%hhu.status",IPC_msg_dec.SDAQ_meas.connected_to_BUS,
																   IPC_msg_dec.SDAQ_meas.SDAQ_serial_number,
																   IPC_msg_dec.SDAQ_meas.channel);
						sprintf(meas_status_str, "%s%s", !IPC_msg_dec.SDAQ_meas.SDAQ_channel_meas.status?"Okay":"No Sensor",
														 IPC_msg_dec.SDAQ_meas.SDAQ_channel_meas.unit<Unit_code_base_region_size ? ", Un-calibrated":"");
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str),
														   meas_status_str,
														   UA_TYPES_STRING);
						sprintf(Node_ID_str, "%s.%d.CH%hhu.meas",IPC_msg_dec.SDAQ_meas.connected_to_BUS,
															     IPC_msg_dec.SDAQ_meas.SDAQ_serial_number,
																 IPC_msg_dec.SDAQ_meas.channel);
						if(IPC_msg_dec.SDAQ_meas.SDAQ_channel_meas.status)
							IPC_msg_dec.SDAQ_meas.SDAQ_channel_meas.meas = NAN;
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str),
														   &(IPC_msg_dec.SDAQ_meas.SDAQ_channel_meas.meas),
														   UA_TYPES_FLOAT);
					pthread_mutex_unlock(&OPC_UA_NODESET_access);
					break;
				case IPC_CAN_BUS_info:
					sprintf(Node_ID_str, "%s.BUS_util", IPC_msg_dec.BUS_info.connected_to_BUS);
					pthread_mutex_lock(&OPC_UA_NODESET_access);
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec.BUS_info.BUS_utilization), UA_TYPES_FLOAT);
					pthread_mutex_unlock(&OPC_UA_NODESET_access);
					break;
				case IPC_SDAQ_register_or_update:
					//printf("Enter:IPC_SDAQ_register_or_update\n");
					SDAQ2OPC_UA_register_update(server, (SDAQ_reg_update_msg*)&IPC_msg_dec);//mutex inside
					break;
				case IPC_SDAQ_clean_up:
					//printf("Enter:IPC_SDAQ_clean_up\n");
					pthread_mutex_lock(&OPC_UA_NODESET_access);
						sprintf(Node_ID_str,"%s.amount",IPC_msg_dec.SDAQ_clean.connected_to_BUS);
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec.SDAQ_clean.t_amount), UA_TYPES_BYTE);
						sprintf(Node_ID_str, "%s.%d", IPC_msg_dec.SDAQ_clean.connected_to_BUS, IPC_msg_dec.SDAQ_clean.SDAQ_serial_number);
						UA_Server_deleteNode(server, UA_NODEID_STRING(1, Node_ID_str), 1);
					pthread_mutex_unlock(&OPC_UA_NODESET_access);
					break;
				case IPC_SDAQ_info:
					//printf("Enter:IPC_SDAQ_info from %s.%d\n",IPC_msg_dec.SDAQ_info.connected_to_BUS,IPC_msg_dec.SDAQ_info.SDAQ_serial_number);
					SDAQ2OPC_UA_register_update_info(server, (SDAQ_info_msg*)&IPC_msg_dec);//mutex inside
					break;
				case IPC_SDAQ_cal_date:
					//printf("Cal date received!!!\n");
					pthread_mutex_lock(&OPC_UA_NODESET_access);
						sprintf(Node_ID_str, "%s.%d.CH%hhu.dates",IPC_msg_dec.SDAQ_meas.connected_to_BUS,
															     IPC_msg_dec.SDAQ_meas.SDAQ_serial_number,
																 IPC_msg_dec.SDAQ_meas.channel);
						calibration_date.day = !IPC_msg_dec.SDAQ_cal_date.SDAQ_cal_date.day?1:IPC_msg_dec.SDAQ_cal_date.SDAQ_cal_date.day;
						calibration_date.month = !IPC_msg_dec.SDAQ_cal_date.SDAQ_cal_date.month?1:IPC_msg_dec.SDAQ_cal_date.SDAQ_cal_date.month;
						calibration_date.year = IPC_msg_dec.SDAQ_cal_date.SDAQ_cal_date.year + 2000;
						cal_time = UA_DateTime_fromStruct(calibration_date);
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str),
														   &cal_time,
														   UA_TYPES_DATETIME);
						sprintf(Node_ID_str, "%s.%d.CH%hhu.period",IPC_msg_dec.SDAQ_meas.connected_to_BUS,
															     IPC_msg_dec.SDAQ_meas.SDAQ_serial_number,
																 IPC_msg_dec.SDAQ_meas.channel);
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str),
														   &(IPC_msg_dec.SDAQ_cal_date.SDAQ_cal_date.period),
														   UA_TYPES_BYTE);
						sprintf(Node_ID_str, "%s.%d.CH%hhu.points",IPC_msg_dec.SDAQ_meas.connected_to_BUS,
																   IPC_msg_dec.SDAQ_meas.SDAQ_serial_number,
																   IPC_msg_dec.SDAQ_meas.channel);
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str),
														   &(IPC_msg_dec.SDAQ_cal_date.SDAQ_cal_date.amount_of_points),
														   UA_TYPES_BYTE);
					pthread_mutex_unlock(&OPC_UA_NODESET_access);
					break;
				case IPC_SDAQ_timediff:
					//printf("Enter:IPC_SDAQ_timediff\n");
					sprintf(Node_ID_str, "%s.%d.TimeDiff", IPC_msg_dec.SDAQ_timediff.connected_to_BUS, IPC_msg_dec.SDAQ_timediff.SDAQ_serial_number);
					pthread_mutex_lock(&OPC_UA_NODESET_access);
						Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec.SDAQ_timediff.Timediff), UA_TYPES_UINT16);
					pthread_mutex_unlock(&OPC_UA_NODESET_access);
					break;
				//--- Message type from any handler (Registration to OPC_UA) ---//
				case IPC_Handler_register:
					//printf("Enter:IPC_Handler_register ");
					switch(IPC_msg_dec.Handler_reg.handler_type)
					{
						case SDAQ:
							SDAQ_handler_reg(server, IPC_msg_dec.Handler_reg.connected_to_BUS);//mutex inside
							break;
						case MDAQ:
							break;
						case IOBOX:
							break;
						case MTI:
							break;
					}
					break;
				case IPC_Handler_unregister:
					//printf("Enter:IPC_Handler_unregister\n");
					pthread_mutex_lock(&OPC_UA_NODESET_access);
						UA_Server_deleteNode(server, UA_NODEID_STRING(1, IPC_msg_dec.Handler_reg.connected_to_BUS), 1);
					pthread_mutex_unlock(&OPC_UA_NODESET_access);
					break;
			}
		}
		if((time(NULL) - health_update_check))
		{
			Rpi_health_update(0);
			time(&health_update_check);
		}
    }
	close(FIFO_fd);
	return NULL;
}

void Morfeas_opc_ua_add_variable_node(UA_Server *server_ptr, char *Parent_id, char *Node_id, char *node_name, int _UA_Type)
{
	UA_VariableAttributes vAttr = UA_VariableAttributes_default;
	vAttr.displayName = UA_LOCALIZEDTEXT("en-US", node_name);
			vAttr.dataType = UA_TYPES[_UA_Type].typeId;
			UA_Server_addVariableNode(server_ptr,
									  UA_NODEID_STRING(1, Node_id),
									  UA_NODEID_STRING(1, Parent_id),
									  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
									  UA_QUALIFIEDNAME(1, Node_id),
									  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
									  vAttr, NULL, NULL);
}

void Morfeas_opc_ua_add_abject_node(UA_Server *server_ptr, char *Parent_id, char *Node_id, char *node_name)
{
	UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", node_name);
    UA_Server_addObjectNode(server_ptr,
    						UA_NODEID_STRING(1, Node_id),
                            UA_NODEID_STRING(1, Parent_id),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, Node_id),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                            oAttr, NULL, NULL);
}

void Rpi_health_update (int sign)
{
	FILE *CPU_temp_fp;
	char cpu_temp_str[20];
	unsigned int Up_time;
	float CPU_Util,RAM_Util,CPU_temp,Disk_Util;
	static glibtop_cpu buff_cpuload_before={0};
	glibtop_cpu buff_cpuload_after={0};
	glibtop_uptime buff_uptime;
	glibtop_mem buff_ram;
	glibtop_fsusage buff_disk;

	//Read values
	glibtop_get_uptime (&buff_uptime);//get computer's Up_time
	glibtop_get_mem (&buff_ram);//get ram util
	glibtop_get_cpu (&buff_cpuload_after);//get cpu util
	glibtop_get_fsusage (&buff_disk,"/");
	//Calc CPU Utilization. Using current and old sample
	CPU_Util=100.0*(buff_cpuload_after.user-buff_cpuload_before.user);
	CPU_Util/=(buff_cpuload_after.total-buff_cpuload_before.total);
	//store current CPU stat sample to old
	memcpy(&buff_cpuload_before,&buff_cpuload_after,sizeof(glibtop_cpu));
	//Calc ram utilization
	RAM_Util=(buff_ram.used - buff_ram.buffer - buff_ram.cached) * 100.0 / buff_ram.total;
	Up_time=buff_uptime.uptime;
	//Calc Disk Utilization
	Disk_Util=(buff_disk.blocks - buff_disk.bavail) * 100.0 / buff_disk.blocks;
	//Read CPU Temp from sysfs
	CPU_temp_fp = fopen(CPU_temp_sysfs_file, "r");
	if(CPU_temp_fp!=NULL)
	{
		fscanf(CPU_temp_fp, "%s", cpu_temp_str);
		fclose(CPU_temp_fp);
		CPU_temp = atof(cpu_temp_str) / 1E3;
	}
	else
		CPU_temp = NAN;
	//Update health_values to OPC_UA mem space
	pthread_mutex_lock(&OPC_UA_NODESET_access);
		Update_NodeValue_by_nodeID(server,UA_NODEID_STRING(1,"Up_time"),&Up_time,UA_TYPES_UINT32);
		Update_NodeValue_by_nodeID(server,UA_NODEID_STRING(1,"CPU_Util"),&CPU_Util,UA_TYPES_FLOAT);
		Update_NodeValue_by_nodeID(server,UA_NODEID_STRING(1,"RAM_Util"),&RAM_Util,UA_TYPES_FLOAT);
		Update_NodeValue_by_nodeID(server,UA_NODEID_STRING(1,"CPU_temp"),&CPU_temp,UA_TYPES_FLOAT);
		Update_NodeValue_by_nodeID(server,UA_NODEID_STRING(1,"Disk_Util"),&Disk_Util,UA_TYPES_FLOAT);
	pthread_mutex_unlock(&OPC_UA_NODESET_access);
}

void Morfeas_opc_ua_root_nodeset_Define(UA_Server *server_ptr)
{
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    //Root of the object "ISO_Channels"
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ISO Channels");
    UA_Server_addObjectNode(server_ptr,
    						UA_NODEID_STRING(1, "ISO_Channels"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "ISO_Channels"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                            oAttr, NULL, NULL);

    //Root of the object "Morfeas_Handlers"
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Interface");
    UA_Server_addObjectNode(server_ptr,
    						UA_NODEID_STRING(1, "Morfeas_Handlers"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "Morfeas_Handlers"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                            oAttr, NULL, NULL);

    //Root of the object "Rpi Health Status"
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "RPi Health status");
    UA_Server_addObjectNode(server_ptr,
    						UA_NODEID_STRING(1, "Health_status"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "Health_status"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                            oAttr, NULL, NULL);

	char *health_status_str[][5]={
		{"Up_time (sec)","CPU_Util (%)","RAM_Util (%)","Disk_Util (%)","CPU_temp (°C)"},
		{"Up_time","CPU_Util","RAM_Util","Disk_Util","CPU_temp"}
	};
	//loop that adding CPU_Temp, UpTime and CPU, RAM and Disk utilization properties;
	for(int i=0; i<5; i++)
		Morfeas_opc_ua_add_variable_node(server_ptr, "Health_status", health_status_str[1][i], health_status_str[0][i], !i?UA_TYPES_UINT32:UA_TYPES_FLOAT);
}

inline void Update_NodeValue_by_nodeID(UA_Server *server_ptr, UA_NodeId Node_to_update, const void *value, int _UA_Type)
{
	UA_Variant temp_value;
	if(_UA_Type!=UA_TYPES_STRING)
		UA_Variant_setScalar(&temp_value, (void *) value, &UA_TYPES[_UA_Type]);

	else
	{
		UA_String str = UA_STRING((char*) value);
		UA_Variant_setScalar(&temp_value, &str, &UA_TYPES[UA_TYPES_STRING]);
    }
	UA_Server_writeValue(server_ptr, Node_to_update, temp_value);
}

