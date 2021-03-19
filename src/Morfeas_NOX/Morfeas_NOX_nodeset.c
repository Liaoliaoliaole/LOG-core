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
		sprintf(Node_ID_str, "%s.Sensors", Dev_or_Bus_name);
		Morfeas_opc_ua_add_object_node(server_ptr, Dev_or_Bus_name, Node_ID_str, "UniNOx");
	pthread_mutex_unlock(&OPC_UA_NODESET_access);
}

void IPC_msg_from_NOX_handler(UA_Server *server, unsigned char type, IPC_message *IPC_msg_dec)
{
	UA_NodeId NodeId;
	UA_DateTime last_seen_time;
	//UA_DateTimeStruct calibration_date = {0};
	char Anchor[30], Node_ID_str[60], meas_status_str[60];
	unsigned char Channel;

	//Msg type from SDAQ_handler
	switch(type)
	{
		case IPC_NOX_data:
			pthread_mutex_lock(&OPC_UA_NODESET_access);
				
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
			pthread_mutex_unlock(&OPC_UA_NODESET_access);
			break;
	}
}