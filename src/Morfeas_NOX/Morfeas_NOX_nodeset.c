/*
File: Morfeas_ΝΟΧ_nodeset.c, implementation of OPC-UA server's
construction/deconstruction functions for Morfeas ΝΟΧ_handler.

Copyright (C) 12021-12022  Sam harry Tzavaras

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
#include <math.h>
#include <arpa/inet.h>

#include <cjson/cJSON.h>
//Include Functions implementation header
#include "../Morfeas_opc_ua/Morfeas_handlers_nodeset.h"

//The DBus method caller function. Return 0 if not internal error.
int Morfeas_DBus_method_call(const char *handler_type, const char *dev_name, const char *method, const char *contents, UA_String *reply);

void NOX_handler_reg(UA_Server *server_ptr, char *Dev_or_Bus_name)
{
	char Node_ID_str[30], Child_Node_ID_str[60];
	pthread_mutex_lock(&OPC_UA_NODESET_access);
		//Add NOX handler root node
		sprintf(Node_ID_str, "%s-if (%s)", Morfeas_IPC_handler_type_name[NOX], Dev_or_Bus_name);
		Morfeas_opc_ua_add_object_node(server_ptr, "NOX-ifs", Dev_or_Bus_name, Node_ID_str);
		sprintf(Node_ID_str, "%s.amount", Dev_or_Bus_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, Dev_or_Bus_name, Node_ID_str, "Dev_on_BUS", UA_TYPES_BYTE);
		sprintf(Node_ID_str, "%s.BUS_util", Dev_or_Bus_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, Dev_or_Bus_name, Node_ID_str, "BUS_Util (%)", UA_TYPES_FLOAT);
		sprintf(Node_ID_str, "%s.BUS_name", Dev_or_Bus_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, Dev_or_Bus_name, Node_ID_str, "CANBus", UA_TYPES_STRING);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,Node_ID_str), Dev_or_Bus_name, UA_TYPES_STRING);
		sprintf(Node_ID_str, "%s.auto_switch_off_value", Dev_or_Bus_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, Dev_or_Bus_name, Node_ID_str, "Auto-off Set (Sec)", UA_TYPES_UINT16);
		sprintf(Node_ID_str, "%s.auto_switch_off_cnt", Dev_or_Bus_name);
		Morfeas_opc_ua_add_variable_node(server_ptr, Dev_or_Bus_name, Node_ID_str, "Auto-off CNT (Sec)", UA_TYPES_UINT16);
		if(!strstr(Dev_or_Bus_name, "vcan"))
		{	//Object with electric status of a SDAQnet port
			sprintf(Node_ID_str, "%s.Electrics", Dev_or_Bus_name);
			Morfeas_opc_ua_add_object_node(server_ptr, Dev_or_Bus_name, Node_ID_str, "Electric");
			sprintf(Child_Node_ID_str, "%s.volts", Dev_or_Bus_name);
			Morfeas_opc_ua_add_variable_node(server_ptr, Node_ID_str, Child_Node_ID_str, "Voltage (V)", UA_TYPES_FLOAT);
			sprintf(Child_Node_ID_str, "%s.amps", Dev_or_Bus_name);
			Morfeas_opc_ua_add_variable_node(server_ptr, Node_ID_str, Child_Node_ID_str, "Amperage (A)", UA_TYPES_FLOAT);
			sprintf(Child_Node_ID_str, "%s.shunt", Dev_or_Bus_name);
			Morfeas_opc_ua_add_variable_node(server_ptr, Node_ID_str, Child_Node_ID_str, "Shunt Temp (°C)", UA_TYPES_FLOAT);
		}
		sprintf(Node_ID_str, "%s.sensors", Dev_or_Bus_name);
		Morfeas_opc_ua_add_object_node(server_ptr, Dev_or_Bus_name, Node_ID_str, "UniNOx");
	pthread_mutex_unlock(&OPC_UA_NODESET_access);
}

void IPC_msg_from_NOX_handler(UA_Server *server, unsigned char type, IPC_message *IPC_msg_dec)
{
	UA_NodeId NodeId;
	char label_str[30], parent_Node_ID_str[60], Node_ID_str[90];

	//Msg type from SDAQ_handler
	switch(type)
	{
		case IPC_NOX_data:
			pthread_mutex_lock(&OPC_UA_NODESET_access);
				/*
				sprintf(Node_ID_str, "%s.sensors.addr_%d", IPC_msg_dec->NOX_BUS_info.Dev_or_Bus_name, i);
				if(!UA_Server_readNodeId(server, UA_NODEID_STRING(1, Node_ID_str), &NodeId))
				{
					UA_Server_deleteNode(server, NodeId, 1);
					UA_clear(&NodeId, &UA_TYPES[UA_TYPES_NODEID]);
				}
				*/
			pthread_mutex_unlock(&OPC_UA_NODESET_access);
			break;
		case IPC_NOX_CAN_BUS_info:
			pthread_mutex_lock(&OPC_UA_NODESET_access);
				sprintf(Node_ID_str, "%s.BUS_util", IPC_msg_dec->NOX_BUS_info.Dev_or_Bus_name);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->NOX_BUS_info.BUS_utilization), UA_TYPES_FLOAT);
				sprintf(Node_ID_str, "%s.amount", IPC_msg_dec->NOX_BUS_info.Dev_or_Bus_name);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->NOX_BUS_info.Dev_on_bus), UA_TYPES_BYTE);
				sprintf(Node_ID_str, "%s.auto_switch_off_value", IPC_msg_dec->NOX_BUS_info.Dev_or_Bus_name);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->NOX_BUS_info.auto_switch_off_value), UA_TYPES_UINT16);
				sprintf(Node_ID_str, "%s.auto_switch_off_cnt", IPC_msg_dec->NOX_BUS_info.Dev_or_Bus_name);
				Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->NOX_BUS_info.auto_switch_off_cnt), UA_TYPES_UINT16);
				if(IPC_msg_dec->NOX_BUS_info.Electrics)
				{
					sprintf(Node_ID_str, "%s.volts", IPC_msg_dec->NOX_BUS_info.Dev_or_Bus_name);
					Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->NOX_BUS_info.voltage), UA_TYPES_FLOAT);
					sprintf(Node_ID_str, "%s.amps", IPC_msg_dec->NOX_BUS_info.Dev_or_Bus_name);
					Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->NOX_BUS_info.amperage), UA_TYPES_FLOAT);
					sprintf(Node_ID_str, "%s.shunt", IPC_msg_dec->NOX_BUS_info.Dev_or_Bus_name);
					Update_NodeValue_by_nodeID(server, UA_NODEID_STRING(1,Node_ID_str), &(IPC_msg_dec->NOX_BUS_info.shunt_temp), UA_TYPES_FLOAT);
				}
				for(int i=0; i<2; i++)
				{
					sprintf(Node_ID_str, "%s.sensors.addr_%d", IPC_msg_dec->NOX_BUS_info.Dev_or_Bus_name, i);
					if(IPC_msg_dec->NOX_BUS_info.active_devs[i])
					{
						if(UA_Server_readNodeId(server, UA_NODEID_STRING(1, Node_ID_str), &NodeId))
						{
							sprintf(parent_Node_ID_str, "%s.sensors", IPC_msg_dec->NOX_BUS_info.Dev_or_Bus_name);
							sprintf(label_str, "UniNOx(Addr:%d)", i);
							Morfeas_opc_ua_add_object_node(server, parent_Node_ID_str, Node_ID_str, label_str);
							//Populate objects and variables for UniNOx sensor's data
							strcpy(parent_Node_ID_str, Node_ID_str);
							sprintf(Node_ID_str, "%s.NOx_value", parent_Node_ID_str);
							Morfeas_opc_ua_add_variable_node(server, parent_Node_ID_str, Node_ID_str, "NOx value (ppm)", UA_TYPES_FLOAT);
							sprintf(Node_ID_str, "%s.O2_value", parent_Node_ID_str);
							Morfeas_opc_ua_add_variable_node(server, parent_Node_ID_str, Node_ID_str, "O2 value (%)", UA_TYPES_FLOAT);
							//Populate objects and variables for UniNOx sensor's status
							sprintf(Node_ID_str, "%s.sensors.addr_%d.status", IPC_msg_dec->NOX_BUS_info.Dev_or_Bus_name, i);
							Morfeas_opc_ua_add_object_node(server, parent_Node_ID_str, Node_ID_str, "Status");
							strcpy(parent_Node_ID_str, Node_ID_str);
							sprintf(Node_ID_str, "%s.meas_state", parent_Node_ID_str);
							Morfeas_opc_ua_add_variable_node(server, parent_Node_ID_str, Node_ID_str, "Measure state", UA_TYPES_BOOLEAN);
							sprintf(Node_ID_str, "%s.supply", parent_Node_ID_str);
							Morfeas_opc_ua_add_variable_node(server, parent_Node_ID_str, Node_ID_str, "Supply in range", UA_TYPES_BOOLEAN);
							sprintf(Node_ID_str, "%s.inTemp", parent_Node_ID_str);
							Morfeas_opc_ua_add_variable_node(server, parent_Node_ID_str, Node_ID_str, "in Temperature", UA_TYPES_BOOLEAN);
							sprintf(Node_ID_str, "%s.NOx_valid", parent_Node_ID_str);
							Morfeas_opc_ua_add_variable_node(server, parent_Node_ID_str, Node_ID_str, "is NOx_value valid", UA_TYPES_BOOLEAN);
							sprintf(Node_ID_str, "%s.O2_valid", parent_Node_ID_str);
							Morfeas_opc_ua_add_variable_node(server, parent_Node_ID_str, Node_ID_str, "is O2_value valid", UA_TYPES_BOOLEAN);
							sprintf(Node_ID_str, "%s.heater_state", parent_Node_ID_str);
							Morfeas_opc_ua_add_variable_node(server, parent_Node_ID_str, Node_ID_str, "Heater state", UA_TYPES_STRING);
							//Populate objects and variables for UniNOx sensor's errors
							sprintf(Node_ID_str, "%s.errors.addr_%d.errors", IPC_msg_dec->NOX_BUS_info.Dev_or_Bus_name, i);
							Morfeas_opc_ua_add_object_node(server, parent_Node_ID_str, Node_ID_str, "Errors");
							strcpy(parent_Node_ID_str, Node_ID_str);
							sprintf(Node_ID_str, "%s.heater_element", parent_Node_ID_str);
							Morfeas_opc_ua_add_variable_node(server, parent_Node_ID_str, Node_ID_str, "Heater element", UA_TYPES_STRING);
							sprintf(Node_ID_str, "%s.NOx_element", parent_Node_ID_str);
							Morfeas_opc_ua_add_variable_node(server, parent_Node_ID_str, Node_ID_str, "NOx element", UA_TYPES_STRING);
							sprintf(Node_ID_str, "%s.O2_element", parent_Node_ID_str);
							Morfeas_opc_ua_add_variable_node(server, parent_Node_ID_str, Node_ID_str, "O2 element", UA_TYPES_STRING);
						}
						else
							UA_clear(&NodeId, &UA_TYPES[UA_TYPES_NODEID]);
					}
					else
					{
						if(!UA_Server_readNodeId(server, UA_NODEID_STRING(1, Node_ID_str), &NodeId))
						{
							UA_Server_deleteNode(server, NodeId, 1);
							UA_clear(&NodeId, &UA_TYPES[UA_TYPES_NODEID]);
						}
					}
				}
			pthread_mutex_unlock(&OPC_UA_NODESET_access);
			break;
	}
}
