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

//OPC_UA local Functions
void Morfeas_opc_ua_nodeset_Define(UA_Server *server);
void Update_NodeValue_by_nodeID(UA_Server *server, UA_NodeId Node_to_update, void * value, int _UA_Type);
//FIFO reader, Thread function.
void* FIFO_Reader(void *varg_pt);
//Timer Handler Function
void timer_handler(int sign);

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


	if(!argv[1])
	{
		fprintf(stderr,"No argument for path to FIFO\n");
		exit(EXIT_FAILURE);
	}

	//Install timer_handler as the signal handler for SIGALRM.
	memset (&tim_sa, 0, sizeof (tim_sa));
	tim_sa.sa_handler = &timer_handler;
	sigaction (SIGALRM, &tim_sa, NULL);

	//Init OPC_UA Server
	server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
	Morfeas_opc_ua_nodeset_Define(server);

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
	char *path_to_FIFO = varg_pt;
    while (running)
	{
		if((type = IPC_msg_RX(path_to_FIFO, &IPC_msg_dec)))
		{
			switch(type)
			{
				case IPC_SDAQ_meas:
					pthread_mutex_lock(&OPC_UA_NODESET_access);
						printf("\nMessage from Bus:%s\n",IPC_msg_dec.SDAQ_meas.connected_to_BUS);
						printf("\tAnchor:%10u.CH%02u\n",IPC_msg_dec.SDAQ_meas.serial_number, IPC_msg_dec.SDAQ_meas.channel);
						printf("\tValue=%9.3f %s\n",IPC_msg_dec.SDAQ_meas.SDAQ_channel_meas.meas,
												    unit_str[IPC_msg_dec.SDAQ_meas.SDAQ_channel_meas.unit]);
						printf("\tTimestamp=%hu\n",IPC_msg_dec.SDAQ_meas.SDAQ_channel_meas.timestamp);
					pthread_mutex_unlock(&OPC_UA_NODESET_access);
					break;
			}
		}
    }
	return NULL;
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

void Morfeas_opc_ua_nodeset_Define(UA_Server *server_ptr)
{
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    //Root of the object "ISO_Channels"
	UA_NodeId ISO_Channels;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ISO Channels");
    UA_Server_addObjectNode(server_ptr,
    						UA_NODEID_STRING(1, "ISO_Channels"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "ISO_Channels"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                            oAttr, NULL, &ISO_Channels);

    //Root of the object "Morfeas_Handlers"
	UA_NodeId Morfeas_Handlers;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Morfeas Handlers");
    UA_Server_addObjectNode(server_ptr,
    						UA_NODEID_STRING(1, "Morfeas_Handlers"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "Morfeas_Handlers"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                            oAttr, NULL, &Morfeas_Handlers);

    //Root of the object "Rpi Health Status"
	UA_NodeId Health_status;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "RPi Health status");
    UA_Server_addObjectNode(server_ptr,
    						UA_NODEID_STRING(1, "Health_status"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "Health_status"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                            oAttr, NULL, &Health_status);

	const char *health_status_str[][5]={
		{"Up_time (sec)","CPU_Util (%)","RAM_Util (%)","Disk_Util (%)","CPU_temp (°C)"},
		{"Up_time","CPU_Util","RAM_Util","Disk_Util","CPU_temp"}
	};

	//loop that adding CPU_Temp, UpTime and CPU, RAM and Disk utilization properties;
	for(int i=0; i<5; i++)
	{
		vAttr.displayName = UA_LOCALIZEDTEXT("en-US", (char *)health_status_str[0][i]);
		vAttr.dataType = i==0?UA_TYPES[UA_TYPES_UINT32].typeId:UA_TYPES[UA_TYPES_FLOAT].typeId;
		UA_Server_addVariableNode(server_ptr,
								  UA_NODEID_STRING(1, (char *)health_status_str[1][i]),
								  UA_NODEID_STRING(1, "Health_status"),//Health_status,
		                          UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
		                          UA_QUALIFIEDNAME(1, (char *)health_status_str[1][i]),
		                          UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
		                          vAttr, NULL, NULL);
	}
}

inline void Update_NodeValue_by_nodeID(UA_Server *server_ptr, UA_NodeId Node_to_update, void *value, int _UA_Type)
{
	UA_Variant temp_value;
    UA_Variant_setScalar(&temp_value, value, &UA_TYPES[_UA_Type]);
    UA_Server_writeValue(server_ptr, Node_to_update, temp_value);
}

