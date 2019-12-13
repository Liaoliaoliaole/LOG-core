/*
File: Morfeas_SDAQ_nodeset.c, implementation of OPC-UA server's Nodeset 
construction/deconstruction functions for Morfeas SDAQ_handler.

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

//Include Functions implementation header
#include "Morfeas_handlers_nodeset.h"

pthread_mutex_t OPC_UA_NODESET_access = PTHREAD_MUTEX_INITIALIZER;

void SDAQ_handler_reg(UA_Server *server_ptr, char *connected_to_BUS)
{
	UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
	UA_VariableAttributes vAttr = UA_VariableAttributes_default;
	char tmp_buff[30];
	sprintf(tmp_buff, "%s-if (%s)", Morfeas_IPC_handler_type_name[SDAQ], connected_to_BUS);
	pthread_mutex_lock(&OPC_UA_NODESET_access);
		oAttr.displayName = UA_LOCALIZEDTEXT("en-US", tmp_buff);
		UA_Server_addObjectNode(server_ptr,
								UA_NODEID_STRING(1, connected_to_BUS),
								UA_NODEID_STRING(1, "Morfeas_Handlers"),
								UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
								UA_QUALIFIEDNAME(1, connected_to_BUS),
								UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
								oAttr, NULL, NULL);
		oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "SDAQnet");
		sprintf(tmp_buff, "%s.SDAQnet", connected_to_BUS);
		UA_Server_addObjectNode(server_ptr,
								UA_NODEID_STRING(1, tmp_buff),
								UA_NODEID_STRING(1, connected_to_BUS),
								UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
								UA_QUALIFIEDNAME(1, tmp_buff),
								UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
								oAttr, NULL, NULL);
		vAttr.displayName = UA_LOCALIZEDTEXT("en-US", "BUS Utilization (%)");
		vAttr.dataType = UA_TYPES[UA_TYPES_FLOAT].typeId;
		sprintf(tmp_buff, "%s.BUS_util", connected_to_BUS);
		UA_Server_addVariableNode(server_ptr,
								  UA_NODEID_STRING(1,tmp_buff),
								  UA_NODEID_STRING(1, connected_to_BUS),
		                          UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
		                          UA_QUALIFIEDNAME(1, tmp_buff),
		                          UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
		                          vAttr, NULL, NULL);
		vAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Amount of SDAQs");
		vAttr.dataType = UA_TYPES[UA_TYPES_BYTE].typeId;
		sprintf(tmp_buff, "%s.amount", connected_to_BUS);
		UA_Server_addVariableNode(server_ptr,
								  UA_NODEID_STRING(1,tmp_buff),
								  UA_NODEID_STRING(1, connected_to_BUS),
		                          UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
		                          UA_QUALIFIEDNAME(1, tmp_buff),
		                          UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
		                          vAttr, NULL, NULL);
	pthread_mutex_unlock(&OPC_UA_NODESET_access);
}

void SDAQ2OPC_UA_register_update(UA_Server *server_ptr, SDAQ_reg_update_msg *ptr)
{
	char SDAQ_anchor_str[15], tmp_str[50], tmp_str2[50];
	UA_NodeId out;
	UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
	sprintf(SDAQ_anchor_str,"%s.%d",ptr->connected_to_BUS,ptr->SDAQ_status.dev_sn);
	sprintf(tmp_str,"%s.SDAQnet",ptr->connected_to_BUS);
	UA_NodeId_init(&out);
	pthread_mutex_lock(&OPC_UA_NODESET_access);
		if(UA_Server_readNodeId(server_ptr, UA_NODEID_STRING(1, SDAQ_anchor_str), &out))
		{
			//printf("Device %s with S/N:%s is not registered!!!\n", tmp_str, SDAQ_anchor_str);
			oAttr.displayName = UA_LOCALIZEDTEXT("en-US", (char *)dev_type_str[ptr->SDAQ_status.dev_type]);
			UA_Server_addObjectNode(server_ptr,
									UA_NODEID_STRING(1, SDAQ_anchor_str),
									UA_NODEID_STRING(1, tmp_str),
									UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
									UA_QUALIFIEDNAME(1, SDAQ_anchor_str),
									UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
									oAttr, NULL, NULL);
			sprintf(tmp_str,"%s.S/N",SDAQ_anchor_str);
			Morfeas_opc_ua_add_variable_node(server_ptr, SDAQ_anchor_str, tmp_str, "S/N", UA_TYPES_UINT32);
			sprintf(tmp_str,"%s.Address",SDAQ_anchor_str);
			Morfeas_opc_ua_add_variable_node(server_ptr, SDAQ_anchor_str, tmp_str, "Address", UA_TYPES_BYTE);
			sprintf(tmp_str,"%s.TimeDiff",SDAQ_anchor_str);
			Morfeas_opc_ua_add_variable_node(server_ptr, SDAQ_anchor_str, tmp_str, "TimeDiff", UA_TYPES_UINT16);
			sprintf(tmp_str2,"%s.Status",SDAQ_anchor_str);
			oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "SDAQ_Status");
			UA_Server_addObjectNode(server_ptr,
									UA_NODEID_STRING(1, tmp_str2),
									UA_NODEID_STRING(1, SDAQ_anchor_str),
									UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
									UA_QUALIFIEDNAME(1, tmp_str2),
									UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
									oAttr, NULL, NULL);

			sprintf(tmp_str,"%s.State",SDAQ_anchor_str);
			Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str, "State", UA_TYPES_STRING);
			sprintf(tmp_str,"%s.inSync",SDAQ_anchor_str);
			Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str, "inSync", UA_TYPES_STRING);
			sprintf(tmp_str,"%s.Error",SDAQ_anchor_str);
			Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str, "Error", UA_TYPES_STRING);
			sprintf(tmp_str,"%s.Mode",SDAQ_anchor_str);
			Morfeas_opc_ua_add_variable_node(server_ptr, tmp_str2, tmp_str, "Mode", UA_TYPES_STRING);
		}
		sprintf(tmp_str,"%s.amount",ptr->connected_to_BUS);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &(ptr->t_amount), UA_TYPES_BYTE);
		sprintf(tmp_str,"%s.S/N",SDAQ_anchor_str);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &(ptr->SDAQ_status.dev_sn), UA_TYPES_UINT32);
		sprintf(tmp_str,"%s.Address",SDAQ_anchor_str);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), &(ptr->address), UA_TYPES_BYTE);
		sprintf(tmp_str,"%s.State",SDAQ_anchor_str);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), status_byte_dec(ptr->SDAQ_status.status, State), UA_TYPES_STRING);
		sprintf(tmp_str,"%s.inSync",SDAQ_anchor_str);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), status_byte_dec(ptr->SDAQ_status.status, In_sync), UA_TYPES_STRING);
		sprintf(tmp_str,"%s.Error",SDAQ_anchor_str);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), status_byte_dec(ptr->SDAQ_status.status, Error), UA_TYPES_STRING);
		sprintf(tmp_str,"%s.Mode",SDAQ_anchor_str);
		Update_NodeValue_by_nodeID(server_ptr, UA_NODEID_STRING(1,tmp_str), status_byte_dec(ptr->SDAQ_status.status, Mode), UA_TYPES_STRING);
	pthread_mutex_unlock(&OPC_UA_NODESET_access);
}