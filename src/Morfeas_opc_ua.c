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

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <glib.h>
#include <glibtop.h>
#include <glibtop/uptime.h>
#include <glibtop/cpu.h>
#include <glibtop/mem.h>
#include <glibtop/fsusage.h>

//Include Functions implementation header
#include "Morfeas_IPC.h"//<-#include "Types.h"

//FIFO reader, Thread function.
void* FIFO_Reader(void *varg_pt);
//Timer Handler Function
void timer_handler(int sign);
//OPC_UA local Functions
void Morfeas_opc_ua_root_nodeset_Define(UA_Server *server);
void Morfeas_opc_ua_add_variable_node(UA_Server *server_ptr, char *Parent_id, char *Node_id, char *node_name, int _UA_Type);
void Update_NodeValue_by_nodeID(UA_Server *server, UA_NodeId Node_to_update, const void * value, int _UA_Type);
	//----Morfeas BUS Handlers----//
//SDAQ's Handler related
void SDAQ_handler_reg(UA_Server *server, char *connected_to_BUS);
void SDAQ2OPC_UA_register_update(UA_Server *server, SDAQ_reg_update_msg *ptr);

//Global variables
static volatile UA_Boolean running = true;
UA_Server *server;
pthread_mutex_t OPC_UA_NODESET_access = PTHREAD_MUTEX_INITIALIZER;

static void stopHandler(int sign)
{
    if(sign==SIGINT)
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
	running = false;
}

int main(int argc, char *argv[])
{
	struct sigaction tim_sa,stop_sa;
	struct itimerval timer;
	UA_StatusCode retval;
	//variables for threads
	pthread_t *Threads_ids;
	unsigned int i, amount_of_threads = 1; //amount_of_threads loaded from the Configuration
	//Install stopHandler as the signal handler for SIGINT and SIGTERM signals.
    memset (&stop_sa, 0, sizeof (stop_sa));
	stop_sa.sa_handler = &stopHandler;
	sigaction (SIGINT, &stop_sa, NULL);
    sigaction (SIGTERM, &stop_sa, NULL);

	//Install timer_handler as the signal handler for SIGALRM.
	memset (&tim_sa, 0, sizeof (tim_sa));
	tim_sa.sa_handler = &timer_handler;
	sigaction (SIGALRM, &tim_sa, NULL);

	//Init OPC_UA Server
	server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
	Morfeas_opc_ua_root_nodeset_Define(server);

	Threads_ids = malloc(sizeof(Threads_ids)*amount_of_threads); //allocate memory for the threads tags
	//Start threads for the FIFO readers
	for(i=0; i<amount_of_threads; i++)
		pthread_create(&Threads_ids[i], NULL, FIFO_Reader, argv[1]);

	// Configure the timer to repeat every 500ms
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 500000;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 500000;
	// Start a timer
	setitimer (ITIMER_REAL, &timer, NULL);

	//Start OPC_UA Server
    retval = UA_Server_run(server, &running);
	//Wait until all thread is ended
	for(i=0; i<amount_of_threads; i++)
		pthread_join(Threads_ids[i], NULL);// wait for threads to finish
    UA_Server_delete(server);
	free(Threads_ids);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

//FIFO reader, Thread function.
void* FIFO_Reader(void *varg_pt)
{
	//Morfeas IPC msg decoder
	IPC_msg IPC_msg_dec;
	unsigned char type;//type of received IPC_msg
	const char *path_to_FIFO = "/tmp/.Morfeas_FIFO";
    while (running)
	{
		if((type = IPC_msg_RX(path_to_FIFO, &IPC_msg_dec)))
		{
			switch(type)
			{
				case IPC_Handler_register:
					printf("Enter:IPC_Handler_register ");
					switch(IPC_msg_dec.Handler_reg.handler_type)
					{
						case SDAQ:
							SDAQ_handler_reg(server, IPC_msg_dec.Handler_reg.connected_to_BUS);
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
					printf("Enter:IPC_Handler_unregister\n");
					pthread_mutex_lock(&OPC_UA_NODESET_access);
						UA_Server_deleteNode(server, UA_NODEID_STRING(1, IPC_msg_dec.Handler_reg.connected_to_BUS), 1);
					pthread_mutex_unlock(&OPC_UA_NODESET_access);
					break;
				case IPC_SDAQ_register_or_update:
					printf("Enter:IPC_SDAQ_register_or_update\n");
					SDAQ2OPC_UA_register_update(server, (SDAQ_reg_update_msg*)&IPC_msg_dec);
					break;
				case IPC_SDAQ_clean_up:
					printf("Enter:IPC_SDAQ_clean_up\n");
					break;
				case IPC_SDAQ_info:
					printf("Enter:IPC_SDAQ_info\n");
					break;
				case IPC_SDAQ_meas:
				/*
					pthread_mutex_lock(&OPC_UA_NODESET_access);
						printf("\nMessage from Bus:%s\n",IPC_msg_dec.SDAQ_meas.connected_to_BUS);
						printf("\tAnchor:%010u.CH%02u\n",IPC_msg_dec.SDAQ_meas.serial_number, IPC_msg_dec.SDAQ_meas.channel);
						printf("\tValue=%9.3f %s\n",IPC_msg_dec.SDAQ_meas.SDAQ_channel_meas.meas,
												    unit_str[IPC_msg_dec.SDAQ_meas.SDAQ_channel_meas.unit]);
						printf("\tTimestamp=%hu\n",IPC_msg_dec.SDAQ_meas.SDAQ_channel_meas.timestamp);
					pthread_mutex_unlock(&OPC_UA_NODESET_access);
				*/
					break;
			}
		}
    }
	return NULL;
}

void SDAQ_handler_reg(UA_Server *server_ptr, char *connected_to_BUS)
{
	UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
	UA_VariableAttributes vAttr = UA_VariableAttributes_default;
	char tmp_buff[30];
	sprintf(tmp_buff, "%s (%s)", Morfeas_IPC_handler_type_name[SDAQ], connected_to_BUS);
	printf("%s\n",tmp_buff);
	pthread_mutex_lock(&OPC_UA_NODESET_access);
		oAttr.displayName = UA_LOCALIZEDTEXT("en-US", tmp_buff);
		UA_Server_addObjectNode(server_ptr,
								UA_NODEID_STRING(1, connected_to_BUS),
								UA_NODEID_STRING(1, "Morfeas_Handlers"),
								UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
								UA_QUALIFIEDNAME(1, connected_to_BUS),
								UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
								oAttr, NULL, NULL);
		oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Devices");
		sprintf(tmp_buff, "Dev_on_%s", connected_to_BUS);
		UA_Server_addObjectNode(server_ptr,
								UA_NODEID_STRING(1, tmp_buff),
								UA_NODEID_STRING(1, connected_to_BUS),
								UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
								UA_QUALIFIEDNAME(1, tmp_buff),
								UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
								oAttr, NULL, NULL);
		vAttr.displayName = UA_LOCALIZEDTEXT("en-US", "BUS Utilization (%)");
		vAttr.dataType = UA_TYPES[UA_TYPES_FLOAT].typeId;
		UA_Server_addVariableNode(server_ptr,
								  UA_NODEID_STRING(1,"BUS_util"),
								  UA_NODEID_STRING(1, connected_to_BUS),
		                          UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
		                          UA_QUALIFIEDNAME(1, "BUS_util"),
		                          UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
		                          vAttr, NULL, NULL);
	pthread_mutex_unlock(&OPC_UA_NODESET_access);
}

void SDAQ2OPC_UA_register_update(UA_Server *server_ptr, SDAQ_reg_update_msg *ptr)
{
	char Serial_number_str[15], tmp_str[50];
	UA_Variant out;
	UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
	sprintf(Serial_number_str,"%d",ptr->SDAQ_status.dev_sn);
	sprintf(tmp_str,"Dev_on_%s",ptr->connected_to_BUS);
	UA_Variant_init(&out);
	pthread_mutex_lock(&OPC_UA_NODESET_access);
		if(UA_Server_readValue(server_ptr, UA_NODEID_STRING(1, Serial_number_str), &out))
		{
			printf("Device %s with S/N:%s is not registered!!!\n", tmp_str, Serial_number_str);
			oAttr.displayName = UA_LOCALIZEDTEXT("en-US", (char *)dev_type_str[ptr->SDAQ_status.dev_type]);
			UA_Server_addObjectNode(server_ptr,
									UA_NODEID_STRING(1, Serial_number_str),
									UA_NODEID_STRING(1, tmp_str),
									UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
									UA_QUALIFIEDNAME(1, Serial_number_str),
									UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
									oAttr, NULL, NULL);
			sprintf(tmp_str,"%s.%s.S/N",ptr->connected_to_BUS,Serial_number_str);
			Morfeas_opc_ua_add_variable_node(server_ptr, Serial_number_str, tmp_str, "S/N", UA_TYPES_UINT32);
			sprintf(tmp_str,"%s.%s.Address",ptr->connected_to_BUS,Serial_number_str);
			Morfeas_opc_ua_add_variable_node(server_ptr, Serial_number_str, tmp_str, "Address", UA_TYPES_BYTE);
			sprintf(tmp_str,"%s.%s.State",ptr->connected_to_BUS,Serial_number_str);
			Morfeas_opc_ua_add_variable_node(server_ptr, Serial_number_str, tmp_str, "State", UA_TYPES_STRING);
			sprintf(tmp_str,"%s.%s.inSync",ptr->connected_to_BUS,Serial_number_str);
			Morfeas_opc_ua_add_variable_node(server_ptr, Serial_number_str, tmp_str, "inSync", UA_TYPES_STRING);
			sprintf(tmp_str,"%s.%s.Error",ptr->connected_to_BUS,Serial_number_str);
			Morfeas_opc_ua_add_variable_node(server_ptr, Serial_number_str, tmp_str, "Error", UA_TYPES_STRING);
			sprintf(tmp_str,"%s.%s.Mode",ptr->connected_to_BUS,Serial_number_str);
			Morfeas_opc_ua_add_variable_node(server_ptr, Serial_number_str, tmp_str, "Mode", UA_TYPES_STRING);			
		}
		sprintf(tmp_str,"%s.%s.S/N",ptr->connected_to_BUS,Serial_number_str);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &(ptr->SDAQ_status.dev_sn), UA_TYPES_UINT32);
		sprintf(tmp_str,"%s.%s.Address",ptr->connected_to_BUS,Serial_number_str);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &(ptr->address), UA_TYPES_BYTE);
		sprintf(tmp_str,"%s.%s.State",ptr->connected_to_BUS,Serial_number_str);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), status_byte_dec(ptr->SDAQ_status.status, State), UA_TYPES_STRING);
		sprintf(tmp_str,"%s.%s.inSync",ptr->connected_to_BUS,Serial_number_str);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), status_byte_dec(ptr->SDAQ_status.status, In_sync), UA_TYPES_STRING);
		sprintf(tmp_str,"%s.%s.Error",ptr->connected_to_BUS,Serial_number_str);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), status_byte_dec(ptr->SDAQ_status.status, Error), UA_TYPES_STRING);
		sprintf(tmp_str,"%s.%s.Mode",ptr->connected_to_BUS,Serial_number_str);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), status_byte_dec(ptr->SDAQ_status.status, Mode), UA_TYPES_STRING);
	pthread_mutex_unlock(&OPC_UA_NODESET_access);
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
void timer_handler (int sign)
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
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Morfeas Handlers");
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

