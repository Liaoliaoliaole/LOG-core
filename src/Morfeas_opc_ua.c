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
#include "Types.h"

//OPC_UA local Functions
void RPi_stat_Define(UA_Server *server);
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

	//Init OPC_UA Server
	server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
	RPi_stat_Define(server);

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
	sdaq_meas meas_dec;
	char anchor_str[20];
	int fifo_fd, select_ret;
	fd_set readCheck;
    fd_set errCheck;
    struct timeval timeout;
	size_t sizeof_sdaq_meas;
	char *path_to_FIFO = varg_pt;

	if(access(path_to_FIFO, F_OK) == -1 )//Make the Named Pipe(FIFO) if is not exist
			mkfifo(path_to_FIFO, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    FD_ZERO(&readCheck);
    FD_ZERO(&errCheck);
    while (running)
	{
		fifo_fd = open(path_to_FIFO, O_RDWR | O_NONBLOCK);
		FD_SET(fifo_fd, &readCheck);
		FD_SET(fifo_fd, &errCheck);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		select_ret = select(fifo_fd+1, &readCheck, NULL, &errCheck, &timeout);
		if (select_ret < 0)
		    perror("Select failed");
		else if (FD_ISSET(fifo_fd, &errCheck))
		    perror("FD error");
		else if (FD_ISSET(fifo_fd, &readCheck))
		{
			read(fifo_fd, &sizeof_sdaq_meas, sizeof(size_t));
			sizeof_sdaq_meas -= read(fifo_fd, anchor_str, 16);
			sizeof_sdaq_meas -= read(fifo_fd, &meas_dec, sizeof_sdaq_meas);
			if(!sizeof_sdaq_meas)
			{
				pthread_mutex_lock(&OPC_UA_NODESET_access);
					printf("\nReceived from Anchor:%s\n",anchor_str);
					printf("\tValue=%9.3f %s\n",meas_dec.meas, unit_str[meas_dec.unit]);
					printf("\tTimestamp=%hu\n",meas_dec.timestamp);
				pthread_mutex_unlock(&OPC_UA_NODESET_access);
			}
		}
		close(fifo_fd);
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

void RPi_stat_Define(UA_Server *server_ptr)
{
    //Root of the object
	UA_NodeId Health_status; /* get the nodeid assigned by the server */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Health_status");
    UA_Server_addObjectNode(server_ptr, UA_NODEID_NULL,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "Health_status"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                            oAttr, NULL, &Health_status);
	//add UpTime property
    UA_VariableAttributes UpT_Attr = UA_VariableAttributes_default;
	UA_Int64 Up_time=0;
    UA_Variant_setScalar(&UpT_Attr.value, &Up_time, &UA_TYPES[UA_TYPES_UINT32]);
    UpT_Attr.displayName = UA_LOCALIZEDTEXT("en-US", "Up_time (sec)");
	UpT_Attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    UA_Server_addVariableNode(server_ptr, UA_NODEID_STRING(1, "Up_time"), Health_status,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Up_time"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), UpT_Attr, NULL, NULL);
	//add CPU utilization property
    UA_VariableAttributes CPU_util_Attr = UA_VariableAttributes_default;
    UA_Float CPU_util = 0;
    UA_Variant_setScalar(&CPU_util_Attr.value, &CPU_util, &UA_TYPES[UA_TYPES_FLOAT]);
    CPU_util_Attr.displayName = UA_LOCALIZEDTEXT("en-US", "CPU_Util (%)");
	CPU_util_Attr.dataType = UA_TYPES[UA_TYPES_FLOAT].typeId;
    UA_Server_addVariableNode(server_ptr, UA_NODEID_STRING(1, "CPU_Util"), Health_status,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "CPU_Util"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), CPU_util_Attr, NULL, NULL);

	//add RAM utilization property
    UA_VariableAttributes RAM_util_Attr = UA_VariableAttributes_default;
    UA_Float RAM_util = 0;
    UA_Variant_setScalar(&RAM_util_Attr.value, &RAM_util, &UA_TYPES[UA_TYPES_FLOAT]);
    RAM_util_Attr.displayName = UA_LOCALIZEDTEXT("en-US", "RAM_Util (%)");
	RAM_util_Attr.dataType = UA_TYPES[UA_TYPES_FLOAT].typeId;
    UA_Server_addVariableNode(server_ptr, UA_NODEID_STRING(1, "RAM_Util"), Health_status,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "RAM_Util"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), RAM_util_Attr, NULL, NULL);
	//add Disk utilization property
    UA_VariableAttributes Disk_Util_Attr = UA_VariableAttributes_default;
    UA_Float Disk_Util = 0;
    UA_Variant_setScalar(&Disk_Util_Attr.value, &Disk_Util, &UA_TYPES[UA_TYPES_FLOAT]);
    Disk_Util_Attr.displayName = UA_LOCALIZEDTEXT("en-US", "Disk_Util (%)");
	Disk_Util_Attr.dataType = UA_TYPES[UA_TYPES_FLOAT].typeId;
    UA_Server_addVariableNode(server_ptr, UA_NODEID_STRING(1, "Disk_Util"), Health_status,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Disk_Util"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), Disk_Util_Attr, NULL, NULL);
	//add CPU Temp property
    UA_VariableAttributes CPU_temp_Attr = UA_VariableAttributes_default;
    UA_Float CPU_temp = 0;
    UA_Variant_setScalar(&CPU_temp_Attr.value, &CPU_temp, &UA_TYPES[UA_TYPES_FLOAT]);
    CPU_temp_Attr.displayName = UA_LOCALIZEDTEXT("en-US", "CPU_temp (°C)");
	CPU_temp_Attr.dataType = UA_TYPES[UA_TYPES_FLOAT].typeId;
    UA_Server_addVariableNode(server_ptr, UA_NODEID_STRING(1, "CPU_temp"), Health_status,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "CPU_temp"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), CPU_temp_Attr, NULL, NULL);

}

inline void Update_NodeValue_by_nodeID(UA_Server *server_ptr, UA_NodeId Node_to_update, void *value, int _UA_Type)
{
	UA_Variant temp_value;
    UA_Variant_setScalar(&temp_value, value, &UA_TYPES[_UA_Type]);
    UA_Server_writeValue(server_ptr, Node_to_update, temp_value);
}

