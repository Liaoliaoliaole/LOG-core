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
#define VERSION "1.0" /*Release Version of Morfeas_opc_ua*/
#define n_threads 2
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
#include "../Supplementary/Morfeas_run_check.h"
#include "Morfeas_handlers_nodeset.h" //<-#include "Morfeas_Types.h"
#include "../Supplementary/Morfeas_XML.h"

//print the Usage manual
void print_usage(char *prog_name);
//IPC_Receiver, Thread function.
void * IPC_Receiver(void *varg_pt);
//Nodeset config XML reader, Thread Function
void * Nodeset_XML_reader(void *varg_pt);
//Timer Handler Function
void Rpi_health_update(void);
//OPC_UA local Functions
void Morfeas_opc_ua_root_nodeset_Define(UA_Server *server);
//Function that adds a child object under the OPC-UA node "ISO_Channels". building according to Wartsila's specification.
void Morfeas_OPC_UA_add_update_ISO_Channel_node(UA_Server *server, xmlNode *node);

//Global variables
static UA_Boolean running = true;
static UA_Server *server = NULL;
static GSList *Links = NULL;

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
	pthread_t Threads_ids[n_threads] = {0};
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
	//Check if program already runs in other instance.
	if(check_already_run(argv[0]))
	{
		fprintf(stderr, "%s Already running !!!\n", argv[0]);
		exit(EXIT_SUCCESS);
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
	pthread_create(&Threads_ids[0], NULL, IPC_Receiver, NULL);//Create Thread for IPC_receiver
	pthread_create(&Threads_ids[1], NULL, Nodeset_XML_reader, ns_config);//Create Thread for Nodeset_XML_reader

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
	{
		pthread_join(Threads_ids[i], NULL);// wait for thread to finish
		pthread_detach(Threads_ids[i]);
	}
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
	};
	printf("%s\nUsage: %s [Options]\n\n%s",preamp, prog_name, manual);
	return;
}

//Nodeset config XML reader, Thread Function
void * Nodeset_XML_reader(void *varg_pt)
{
	GSList *t_list_ptr;
	struct Link_entry *list_data;
	xmlDoc *doc;//XML tree pointer
	xmlNode *xml_node, *root_element; //XML root Node
	char *ns_config = varg_pt;
	struct stat nsconf_xml_stat;
	if(!ns_config || access(ns_config, R_OK | F_OK ))
	{
		UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
		"Path to Configuration XML file is invalid. Server will run in compatible mode");
		return NULL;
	}
	time_t file_last_mod = 0;
	while(running)
	{
		if(ns_config)
		{
			if(!stat(ns_config, &nsconf_xml_stat))
			{
				if(nsconf_xml_stat.st_mtime - file_last_mod)
				{
					UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Configuration XML File Updated!!!!");
					if(!Morfeas_XML_parsing(ns_config, &doc))
					{
						root_element = xmlDocGetRootElement(doc);
						if(!Morfeas_opc_ua_config_valid(root_element))
						{
							pthread_mutex_lock(&OPC_UA_NODESET_access);
								//Find nodes that going to be remove
								Morfeas_OPC_UA_calc_diff_of_ISO_Channel_node(root_element, &Links);
								t_list_ptr = Links;
								//Remove diff nodes from OPC_UA NODESet
								while(t_list_ptr)
								{
									list_data = t_list_ptr->data;
									UA_Server_deleteNode(server, UA_NODEID_STRING(1, list_data->ISO_channel_name), 1);
									t_list_ptr = t_list_ptr->next;
								}
								//Copy Link's data from xmlDoc to List Links
								XML_doc_to_List_ISO_Channels(root_element, &Links);
								//Add and/or Update OPC_UA NODESet
								for(xml_node = root_element->children; xml_node; xml_node = xml_node->next)
									Morfeas_OPC_UA_add_update_ISO_Channel_node(server, xml_node);
							pthread_mutex_unlock(&OPC_UA_NODESET_access);
						}
						else
							UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
							"Data Validation of The OPC-UA Nodeset configuration XML file failed!!!");
						xmlFreeDoc(doc);//Free XML Doc
					}
				}
				file_last_mod = nsconf_xml_stat.st_mtime;
			}
			else
				perror("Error on get stats for Nodeset configuration XML");
		}
		//Check for file update after 1 sec
		sleep(1);
	}
	g_slist_free_full(Links, free_Link_entry);//Free List Links
	return NULL;
}

//IPC_Receiver, Thread function.
void* IPC_Receiver(void *varg_pt)
{
	char str_msg_buff[128];
	//Morfeas IPC msg decoder
	IPC_message IPC_msg_dec;
	time_t last_health_update=0, now;
	unsigned char type;//type of received IPC_msg
	int FIFO_fd;
	//Make the Named Pipe(FIFO)
	mkfifo(Data_FIFO, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    FIFO_fd = open(Data_FIFO, O_RDWR);// O_NONBLOCK | O_RSYNC, O_RDONLY
	while(running)
	{
		type = IPC_msg_RX(FIFO_fd, &IPC_msg_dec);
		if(type && type<=Morfeas_IPC_MAX_type)
		{
			switch(type)
			{
				//--- Message type from any handler (Registration to OPC_UA) ---//
				case IPC_Handler_register:
					sprintf(str_msg_buff, "Register %s Handler for %s", Morfeas_IPC_handler_type_name[IPC_msg_dec.Handler_reg.handler_type],
																					    IPC_msg_dec.Handler_reg.connected_to_BUS);
					UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, str_msg_buff);
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
					sprintf(str_msg_buff, "Remove %s Handler for %s", Morfeas_IPC_handler_type_name[IPC_msg_dec.Handler_reg.handler_type],
																					    IPC_msg_dec.Handler_reg.connected_to_BUS);
					UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, str_msg_buff);
					pthread_mutex_lock(&OPC_UA_NODESET_access);
						UA_Server_deleteNode(server, UA_NODEID_STRING(1, IPC_msg_dec.Handler_reg.connected_to_BUS), 1);
					pthread_mutex_unlock(&OPC_UA_NODESET_access);
					break;
				default://Msg from Handler (nested if to find from which one)
					if(type>=Morfeas_IPC_SDAQ_MIN_type && type<=Morfeas_IPC_SDAQ_MAX_type)//Msg type from SDAQ_handler
						IPC_msg_from_SDAQ_handler(server, type, &IPC_msg_dec);//mutex inside

					break;
			}
		}
		// Every 1 sec update RPi Health stats
		if((time(&now) - last_health_update))
		{
			Rpi_health_update();
			last_health_update = now;
		}
    }
	close(FIFO_fd);
	return NULL;
}

int Morfeas_ISO_Channels_request_dec(const UA_NodeId *nodeId, char **ISO_Channel, char **req_value)
{
	static char NodeID_str[128];
	size_t str_size;
	if(nodeId->identifierType == UA_NODEIDTYPE_STRING)
	{
		if(nodeId->identifier.string.data && nodeId->identifier.string.length)
		{
			str_size = nodeId->identifier.string.length<sizeof(NodeID_str)?nodeId->identifier.string.length:sizeof(NodeID_str);
			memcpy(NodeID_str, nodeId->identifier.string.data, str_size);
			NodeID_str[str_size] = '\0';
			*ISO_Channel = strtok(NodeID_str, ".");
			*req_value = strtok(NULL, ".");
			if(*ISO_Channel && *req_value)
				return 0;
		}
	}
	return -1;
}

//Function used onRead of DataSourceVariables, Channel related
UA_StatusCode CH_update_value(UA_Server *server_ptr,
						  const UA_NodeId *sessionId, void *sessionContext,
						  const UA_NodeId *nodeId, void *nodeContext,
						  UA_Boolean sourceTimeStamp, const UA_NumericRange *range,
						  UA_DataValue *dataValue)
{
	GSList *List_Links_Node;
	struct Link_entry *Node_data;
	//UA_Variant outValue;
	UA_NodeId src_NodeId;
	char *ISO_Channel, *req_value, src_NodeId_str[128];
	if(nodeId->identifierType == UA_NODEIDTYPE_STRING)
	{
		if(!Morfeas_ISO_Channels_request_dec(nodeId, &ISO_Channel, &req_value))
		{
			List_Links_Node = g_slist_find_custom(Links, ISO_Channel, List_Links_cmp);
			if(List_Links_Node)
			{
				Node_data = List_Links_Node->data;
				//check if the source node exist
				sprintf(src_NodeId_str, "%s.%u.CH%hhu.%s", Node_data->interface_type, Node_data->identifier, Node_data->channel, req_value);
				if(!UA_Server_readNodeId(server_ptr, UA_NODEID_STRING(1, src_NodeId_str), &src_NodeId))
				{
					UA_Server_readValue(server_ptr, src_NodeId, &(dataValue->value));//Get requested Value and write on dataValue
					UA_clear(&src_NodeId, &UA_TYPES[UA_TYPES_NODEID]);
					dataValue->hasValue = true;
				}
			}
			else
				dataValue->hasValue = false;
		}
	}
	return UA_STATUSCODE_GOOD;
}

//Function used onRead of DataSourceVariables, Device related
UA_StatusCode Dev_update_value(UA_Server *server_ptr,
						  const UA_NodeId *sessionId, void *sessionContext,
						  const UA_NodeId *nodeId, void *nodeContext,
						  UA_Boolean sourceTimeStamp, const UA_NumericRange *range,
						  UA_DataValue *dataValue)
{
	GSList *List_Links_Node;
	struct Link_entry *Node_data;
	UA_NodeId src_NodeId;
	char *ISO_Channel, *req_value, src_NodeId_str[128];
	if(nodeId->identifierType == UA_NODEIDTYPE_STRING)
	{
		if(!Morfeas_ISO_Channels_request_dec(nodeId, &ISO_Channel, &req_value))
		{
			List_Links_Node = g_slist_find_custom(Links, ISO_Channel, List_Links_cmp);
			if(List_Links_Node)
			{
				Node_data = List_Links_Node->data;
				//check if the source node exist
				sprintf(src_NodeId_str, "%s.%u.%s", Node_data->interface_type, Node_data->identifier, req_value);
				if(!UA_Server_readNodeId(server_ptr, UA_NODEID_STRING(1, src_NodeId_str), &src_NodeId))
				{
					UA_Server_readValue(server_ptr, src_NodeId, &(dataValue->value));//Get requested Value and write it to dataValue
					UA_clear(&src_NodeId, &UA_TYPES[UA_TYPES_NODEID]);
					dataValue->hasValue = true;
				}
			}
			else
				dataValue->hasValue = false;
		}
	}
	return UA_STATUSCODE_GOOD;
}

//Function used onRead of DataSourceVariables, ISO_channel Status
UA_StatusCode Status_update_value(UA_Server *server_ptr,
						  const UA_NodeId *sessionId, void *sessionContext,
						  const UA_NodeId *nodeId, void *nodeContext,
						  UA_Boolean sourceTimeStamp, const UA_NumericRange *range,
						  UA_DataValue *dataValue)
{
	GSList *List_Links_Node;
	struct Link_entry *Node_data;
	UA_NodeId src_NodeId;
	UA_String t_str;
	char *ISO_Channel, *req_value, src_NodeId_str[128];
	if(nodeId->identifierType == UA_NODEIDTYPE_STRING)
	{
		if(!Morfeas_ISO_Channels_request_dec(nodeId, &ISO_Channel, &req_value))
		{
			List_Links_Node = g_slist_find_custom(Links, ISO_Channel, List_Links_cmp);
			if(List_Links_Node)
			{
				Node_data = List_Links_Node->data;
				//check if the source node exist
				sprintf(src_NodeId_str, "%s.%u.CH%hhu.%s", Node_data->interface_type, Node_data->identifier, Node_data->channel, req_value);
				if(!UA_Server_readNodeId(server_ptr, UA_NODEID_STRING(1, src_NodeId_str), &src_NodeId))
				{
					UA_Server_readValue(server_ptr, src_NodeId, &(dataValue->value));//Get requested Value and write it to dataValue
					UA_clear(&src_NodeId, &UA_TYPES[UA_TYPES_NODEID]);
				}
				else
				{
					if(!strcmp(req_value, "status"))
					{
						t_str = UA_String_fromChars("OFF-Line");
						UA_Variant_setScalarCopy(&(dataValue->value), &t_str, &UA_TYPES[UA_TYPES_STRING]);
						UA_clear(&t_str, &UA_TYPES[UA_TYPES_STRING]);
					}
					else if(!strcmp(req_value, "status_byte"))
					{
						const unsigned char OFFLine_val = -1;
						UA_Variant_setScalarCopy(&(dataValue->value), &OFFLine_val, &UA_TYPES[UA_TYPES_BYTE]);
					}
				}
				dataValue->hasValue = true;
			}
		}
	}
	return UA_STATUSCODE_GOOD;
}

void Morfeas_OPC_UA_add_update_ISO_Channel_node(UA_Server *server_ptr, xmlNode *node)
{
	char tmp_str[50], *ISO_channel_name, *anchor_dec;
	float t_min_max;
	unsigned int ID;
	unsigned char CH;
	UA_NodeId out;
	UA_NodeId_init(&out);
	if(!(ISO_channel_name = XML_node_get_content(node, "ISO_CHANNEL")))
		return;
	if(UA_Server_readNodeId(server_ptr, UA_NODEID_STRING(1, ISO_channel_name), &out))
	{
		Morfeas_opc_ua_add_object_node(server_ptr, "ISO_Channels", ISO_channel_name, ISO_channel_name);
			//---Variables with update from Morfeas_ifs---//
		//Status
		sprintf(tmp_str,"%s.status",ISO_channel_name);
		Morfeas_opc_ua_add_variable_node_with_callback_onRead(server_ptr, ISO_channel_name, tmp_str, "Status", UA_TYPES_STRING, Status_update_value);
		sprintf(tmp_str,"%s.status_byte",ISO_channel_name);
		Morfeas_opc_ua_add_variable_node_with_callback_onRead(server_ptr, ISO_channel_name, tmp_str, "Status value", UA_TYPES_BYTE, Status_update_value);

		//Channel related
		sprintf(tmp_str,"%s.Cal_date",ISO_channel_name);
		Morfeas_opc_ua_add_variable_node_with_callback_onRead(server_ptr, ISO_channel_name, tmp_str, "Calibration Date", UA_TYPES_DATETIME, CH_update_value);
		sprintf(tmp_str,"%s.period",ISO_channel_name);
		Morfeas_opc_ua_add_variable_node_with_callback_onRead(server_ptr, ISO_channel_name, tmp_str, "Calibration Period (Months)", UA_TYPES_BYTE, CH_update_value);
		sprintf(tmp_str,"%s.meas",ISO_channel_name);
		Morfeas_opc_ua_add_variable_node_with_callback_onRead(server_ptr, ISO_channel_name, tmp_str, "Value", UA_TYPES_FLOAT, CH_update_value);
		sprintf(tmp_str,"%s.unit",ISO_channel_name);
		Morfeas_opc_ua_add_variable_node_with_callback_onRead(server_ptr, ISO_channel_name, tmp_str, "Unit", UA_TYPES_STRING, CH_update_value);

		//Device related
		sprintf(tmp_str,"%s.Address",ISO_channel_name);
		Morfeas_opc_ua_add_variable_node_with_callback_onRead(server_ptr, ISO_channel_name, tmp_str, "Device Address", UA_TYPES_BYTE, Dev_update_value);
		sprintf(tmp_str,"%s.onBus",ISO_channel_name);
		Morfeas_opc_ua_add_variable_node_with_callback_onRead(server_ptr, ISO_channel_name, tmp_str, "Found on BUS", UA_TYPES_STRING, Dev_update_value);
		sprintf(tmp_str,"%s.Samplerate",ISO_channel_name);
		Morfeas_opc_ua_add_variable_node_with_callback_onRead(server_ptr, ISO_channel_name, tmp_str, "Samplerate", UA_TYPES_BYTE, Dev_update_value);
		sprintf(tmp_str,"%s.Type",ISO_channel_name);
		Morfeas_opc_ua_add_variable_node_with_callback_onRead(server_ptr, ISO_channel_name, tmp_str, "Device Type", UA_TYPES_STRING, Dev_update_value);

		//Regular variables
		sprintf(tmp_str,"%s.desc",ISO_channel_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, ISO_channel_name, tmp_str, "Description", UA_TYPES_STRING);
		sprintf(tmp_str,"%s.min",ISO_channel_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, ISO_channel_name, tmp_str, "Min", UA_TYPES_FLOAT);
		sprintf(tmp_str,"%s.max",ISO_channel_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, ISO_channel_name, tmp_str, "Max", UA_TYPES_FLOAT);
		sprintf(tmp_str,"%s.id",ISO_channel_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, ISO_channel_name, tmp_str, "Identifier", UA_TYPES_UINT32);
		sprintf(tmp_str,"%s.channel",ISO_channel_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, ISO_channel_name, tmp_str, "Channel", UA_TYPES_BYTE);
	}
	else
		UA_clear(&out, &UA_TYPES[UA_TYPES_NODEID]);
	//Decode Anchor from the XML_doc tree to it's elements
	anchor_dec = XML_node_get_content(node, "ANCHOR");
	sscanf(anchor_dec, "%u.CH%hhu", &ID, &CH);
	//Update values of regular variables with data from Configuration XML
	sprintf(tmp_str,"%s.desc",ISO_channel_name);
	Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), XML_node_get_content(node, "DESCRIPTION"), UA_TYPES_STRING);
	sprintf(tmp_str,"%s.min",ISO_channel_name);
	t_min_max = atof(XML_node_get_content(node, "MIN"));
	Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &t_min_max, UA_TYPES_FLOAT);
	sprintf(tmp_str,"%s.max",ISO_channel_name);
	t_min_max = atof(XML_node_get_content(node, "MAX"));
	Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str),  &t_min_max, UA_TYPES_FLOAT);
	sprintf(tmp_str,"%s.id",ISO_channel_name);
	Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &ID, UA_TYPES_UINT32);
	sprintf(tmp_str,"%s.channel",ISO_channel_name);
	Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &CH, UA_TYPES_BYTE);
}

void Morfeas_opc_ua_add_object_node(UA_Server *server_ptr, char *Parent_id, char *Node_id, char *node_name)
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

void Morfeas_opc_ua_add_variable_node(UA_Server *server_ptr, char *Parent_id, char *Node_id, char *node_name, int _UA_Type)
{
	UA_VariableAttributes vAttr = UA_VariableAttributes_default;
	vAttr.displayName = UA_LOCALIZEDTEXT("en-US", node_name);
	vAttr.accessLevel = UA_ACCESSLEVELMASK_READ;
	vAttr.dataType = UA_TYPES[_UA_Type].typeId;
	UA_Server_addVariableNode(server_ptr,
							  UA_NODEID_STRING(1, Node_id),
							  UA_NODEID_STRING(1, Parent_id),
							  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
							  UA_QUALIFIEDNAME(1, Node_id),
							  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
							  vAttr, NULL, NULL);
}

void Morfeas_opc_ua_add_variable_node_with_callback_onRead(UA_Server *server_ptr, char *Parent_id, char *Node_id, char *node_name, int _UA_Type, void *call_func)
{
	UA_VariableAttributes vAttr = UA_VariableAttributes_default;
	vAttr.displayName = UA_LOCALIZEDTEXT("en-US", node_name);
	vAttr.accessLevel = UA_ACCESSLEVELMASK_READ;
	vAttr.dataType = UA_TYPES[_UA_Type].typeId;
	UA_DataSource DataSource = {0};
	DataSource.read = call_func;
	UA_Server_addDataSourceVariableNode(
							  server_ptr,
							  UA_NODEID_STRING(1, Node_id),
							  UA_NODEID_STRING(1, Parent_id),
							  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
							  UA_QUALIFIEDNAME(1, Node_id),
							  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
							  vAttr, DataSource, NULL, NULL);
}

inline void Update_NodeValue_by_nodeID(UA_Server *server_ptr, UA_NodeId Node_to_update, const void *value, int _UA_Type)
{
	UA_Variant temp_value;
	if(_UA_Type!=UA_TYPES_STRING)
		UA_Variant_setScalar(&temp_value, (void *)value, &UA_TYPES[_UA_Type]);
	else
	{
		UA_String str = UA_STRING((char*) value);
		UA_Variant_setScalar(&temp_value, &str, &UA_TYPES[UA_TYPES_STRING]);
    }
	UA_Server_writeValue(server_ptr, Node_to_update, temp_value);
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
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Interfaces");
    UA_Server_addObjectNode(server_ptr,
    						UA_NODEID_STRING(1, "Morfeas_Handlers"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "Morfeas_Handlers"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                            oAttr, NULL, NULL);
	//Add SDAQ-if object node under Morfeas_Handlers
	Morfeas_opc_ua_add_object_node(server_ptr, "Morfeas_Handlers", "SDAQ-ifs", "SDAQ-ifs");
	//Add MDAQ-if object node under Morfeas_Handlers
	Morfeas_opc_ua_add_object_node(server_ptr, "Morfeas_Handlers", "MDAQ-ifs", "MDAQ-ifs");
	//Add IOBOX-if object node under Morfeas_Handlers
	Morfeas_opc_ua_add_object_node(server_ptr, "Morfeas_Handlers", "IOBOX-ifs", "IOBOX-ifs");
	//Add MTI-if object node under Morfeas_Handlers
	Morfeas_opc_ua_add_object_node(server_ptr, "Morfeas_Handlers", "MTI-ifs", "MTI-ifs");
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

void Rpi_health_update (void)
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

