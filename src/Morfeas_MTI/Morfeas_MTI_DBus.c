/*
File "Morfeas_MTI_DBus.c" Implementation of D-Bus listener for the MTI handler.
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

#define MORFEAS_DBUS_NAME_PROTO "org.freedesktop.Morfeas.MTI."
#define IF_NAME_PROTO "Morfeas.MTI."

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <dbus/dbus.h>

#include <modbus.h>
#include <cjson/cJSON.h>

#include "../Morfeas_Types.h"
#include "../Supplementary/Morfeas_Logger.h"

//External Global variables from Morfeas_MTI_if.c
extern volatile unsigned char handler_run;
extern pthread_mutex_t MTI_access;

	//--- Local Enumerators and constants---//
//static const char *const OBJECT_PATH_NAME = "/Morfeas/MTI/DBUS_server_app";
static const char *const Method[] = {"new_MTI_config", "MTI_Global_SWs", "new_PWM_config", "ctrl_tele_SWs", "echo", NULL};
enum Method_enum{new_MTI_config, MTI_Global_SWs, new_PWM_config, ctrl_tele_SWs, echo};
static DBusError dbus_error;

	//--- MTI's Write Functions ---//
//MTI function that sending a new Radio configuration. Return 0 on success, errno otherwise.
int set_MTI_Radio_config(modbus_t *ctx, unsigned char new_RF_CH, unsigned char new_mode, union MTI_specific_regs *new_sregs);
//MTI function that write the Global power switch. Return 0 on success, errno otherwise.
int set_MTI_Global_switches(modbus_t *ctx, bool global_power, bool global_sleep);
//MTI function that write a new configuration for PWM generators, Return 0 on success, errno otherwise.
int set_MTI_PWM_gens(modbus_t *ctx, struct Gen_config_struct *new_Config);
//MTI function that controlling the state of a controllable telemetry(RMSW, MUX, Mini), Return 0 on success, errno otherwise.
int ctrl_tele_switch(modbus_t *ctx, unsigned char mem_pos, unsigned char tele_type, unsigned char sw_name, bool new_state);

	//--- Local DBus Error Logging function ---//
void Log_DBus_error(char *str);
int DBus_reply_msg(DBusConnection *conn, DBusMessage *msg, char *reply_str);
int DBus_reply_msg_with_error(DBusConnection *conn, DBusMessage *msg, char *reply_str);

	//--- Local Functions ---//
char * new_MTI_config_argValidator(cJSON *JSON_args, unsigned char *new_RF_CH, unsigned char *new_mode, union MTI_specific_regs *sregs);
char * ctrl_tele_SWs_argValidator(cJSON *JSON_args, unsigned char *mem_pos, unsigned char *tele_type, unsigned char *sw_name);
char * new_PWM_config_argValidator(cJSON *JSON_args, struct Gen_config_struct PWM_gens_config[]);

//D-Bus listener function
void * MTI_DBus_listener(void *varg_pt)//Thread function.
{
	//Decoded variables from passer
	modbus_t *ctx = *(((struct MTI_DBus_thread_arguments_passer *)varg_pt)->ctx);
	struct Morfeas_MTI_if_stats *stats = ((struct MTI_DBus_thread_arguments_passer *)varg_pt)->stats;
	//JSON objects
	cJSON *JSON_args = NULL;
	//D-Bus related variables
	char *dbus_server_name_if, *param;
	int i, err;
	DBusConnection *conn;
	DBusMessage *msg;
	DBusMessageIter call_args;
	//Local variables and structures
	char *err_str;
	unsigned char new_RF_CH, new_mode, mem_pos, tele_type, sw_name;
	union MTI_specific_regs sregs;
	struct Gen_config_struct PWM_gens_config[2];

	if(!handler_run)//Immediately exit if called with MTI handler under termination
		return NULL;

	dbus_error_init (&dbus_error);
	Logger("Thread for D-Bus listener Started\n");
	//Connects to a bus daemon and registers with it.
	if(!(conn=dbus_bus_get(DBUS_BUS_SYSTEM, &dbus_error)))
	{
		Log_DBus_error("dbus_bus_get() Failed!!!");
		return NULL;
	}
	//Allocate space and create dbus_server_name
	if(!(dbus_server_name_if = calloc(sizeof(char), strlen(MORFEAS_DBUS_NAME_PROTO)+strlen(stats->dev_name)+1)))
	{
		fprintf(stderr,"Memory error!!!\n");
		exit(EXIT_FAILURE);
	}
	sprintf(dbus_server_name_if, "%s%s", MORFEAS_DBUS_NAME_PROTO, stats->dev_name);
	Logger("Thread's DBus_Name:\"%s\"\n", dbus_server_name_if);

    i = dbus_bus_request_name(conn, dbus_server_name_if, DBUS_NAME_FLAG_DO_NOT_QUEUE, &dbus_error);
    if(dbus_error_is_set (&dbus_error))
        Log_DBus_error("dbus_bus_request_name() Failed!!!");

    if(i != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        Logger("DBus: Name not primary owner, ret = %d\n", i);
        return NULL;
    }
	free(dbus_server_name_if);

	//Allocate space and create dbus_server_if
	if(!(dbus_server_name_if = calloc(sizeof(char), strlen(IF_NAME_PROTO)+strlen(stats->dev_name)+1)))
	{
		fprintf(stderr,"Memory error!!!\n");
		exit(EXIT_FAILURE);
	}
	sprintf(dbus_server_name_if, "%s%s", IF_NAME_PROTO, stats->dev_name);
	Logger("\t Interface:\"%s\"\n", dbus_server_name_if);

	while(handler_run)
	{
		//Wait for incoming messages, timeout in 1 sec
        if(dbus_connection_read_write_dispatch(conn, 1000))
		{
			if((msg = dbus_connection_pop_message(conn)))
			{
				//Analyze received message
				i=0;
				while(Method[i])
				{
					if(dbus_message_is_method_call(msg, dbus_server_name_if, Method[i]))//Validate the method of the call
					{
						//Read the arguments of the call
						if (!dbus_message_iter_init(msg, &call_args))//Validate for zero amount of arguments
						  DBus_reply_msg(conn, msg, "Zero arguments!!!");
						else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&call_args))//Validate for not string argument
						  DBus_reply_msg(conn, msg, "Argument is NOT a string!!!");
						else
						{
							dbus_message_iter_get_basic(&call_args, &param);//Get first argument.
							if(i==echo)//Check for echo method
							{
								DBus_reply_msg(conn, msg, param);
								break;
							}
							if(!(JSON_args = cJSON_Parse(param)))
							{
								DBus_reply_msg(conn, msg, "JSON Parsing failed!!!");
								break;
							}
							err = 0;//Reset error state
							switch(i)//Method execution
							{
								case new_MTI_config:
									if((err_str = new_MTI_config_argValidator(JSON_args, &new_RF_CH, &new_mode, &sregs)))
									{
										DBus_reply_msg(conn, msg, err_str);
										break;
									}
									pthread_mutex_lock(&MTI_access);
										if(!(err = stats->error))
										{	//Sent config to MTI
											if(!(err = set_MTI_Radio_config(ctx,
																			new_RF_CH,
																			new_mode,
																			&sregs)))
												DBus_reply_msg(conn, msg, "new_MTI_config(): Success");
										}
									pthread_mutex_unlock(&MTI_access);
									break;
								case MTI_Global_SWs:
									if(!cJSON_HasObjectItem(JSON_args,"G_P_state") || !cJSON_HasObjectItem(JSON_args,"G_S_state"))
									{
										DBus_reply_msg(conn, msg, "MTI_Global_SWs(): Missing Arguments");
										break;
									}
									pthread_mutex_lock(&MTI_access);
										if(stats->MTI_Radio_config.Tele_dev_type == RM_SW_MUX)
										{
											if(!(err = stats->error))
											{
												if(!(err = set_MTI_Global_switches(ctx,
																				   cJSON_GetObjectItem(JSON_args,"G_P_state")->valueint?1:0,
																				   cJSON_GetObjectItem(JSON_args,"G_S_state")->valueint?1:0)))
													DBus_reply_msg(conn, msg, "MTI_Global_SWs(): Success");
											}
										}
										else
											DBus_reply_msg(conn, msg, "MTI_Global_SWs(): Mode isn't RMSW/MUX");
									pthread_mutex_unlock(&MTI_access);
									break;
								case ctrl_tele_SWs:
									if((err_str = ctrl_tele_SWs_argValidator(JSON_args, &mem_pos, &tele_type, &sw_name)))
									{
										DBus_reply_msg(conn, msg, err_str);
										break;
									}
									pthread_mutex_lock(&MTI_access);
										if(stats->MTI_Radio_config.Tele_dev_type == RM_SW_MUX)
										{
											if(!(err = stats->error))
											{
												if(!(err = ctrl_tele_switch(ctx,
																			mem_pos,
																			tele_type,
																			sw_name,
																			cJSON_GetObjectItem(JSON_args,"new_state")->valueint?1:0)))
													DBus_reply_msg(conn, msg, "ctrl_tele_SWs() Success");
											}
											if(err == MTI_TELE_MODE_ERROR)
											{
												DBus_reply_msg(conn, msg, "ctrl_tele_SWs(): tele_type[mem_pos] != tele_type");
												err = 0;
											}
										}
										else
											DBus_reply_msg(conn, msg, "ctrl_tele_SWs(): Mode isn't RMSW/MUX");
									pthread_mutex_unlock(&MTI_access);
									break;
								case new_PWM_config:
									if((err_str = new_PWM_config_argValidator(JSON_args, PWM_gens_config)))
									{
										DBus_reply_msg(conn, msg, err_str);
										break;
									}
									pthread_mutex_lock(&MTI_access);
										if(stats->MTI_Radio_config.Tele_dev_type == Tele_quad)
										{
											if(!(err = stats->error))
											{
												if(!(err = set_MTI_PWM_gens(ctx, PWM_gens_config)))
													DBus_reply_msg(conn, msg, "new_PWM_config() Success");
											}
										}
										else
											DBus_reply_msg(conn, msg, "new_PWM_config(): Mode isn't 2CH_QUAD");
									pthread_mutex_unlock(&MTI_access);
									break;
							}
							if(err)//if true, MTI in Error
								DBus_reply_msg(conn, msg, "MODBUS Error!!!");
							cJSON_Delete(JSON_args);
						}
						break;
					}
					i++;
				}
				dbus_message_unref(msg);
			}
		}
	}
	//Free memory
	dbus_error_free(&dbus_error);
	free(dbus_server_name_if);
	//dbus_connection_close(conn);
	Logger("D-Bus listener thread terminated\n");
	return NULL;
}

void Log_DBus_error(char *str)
{
    Logger("%s: %s\n", str, dbus_error.message);
    dbus_error_free (&dbus_error);
}

int DBus_reply_msg(DBusConnection *conn, DBusMessage *msg, char *reply_str)
{
	DBusMessage *reply;
	DBusMessageIter reply_args;
	//Send reply
	if(!(reply = dbus_message_new_method_return(msg)))
	{
		Logger("Error in dbus_message_new_method_return()\n");
		return EXIT_FAILURE;
	}

	dbus_message_iter_init_append(reply, &reply_args);

	if (!dbus_message_iter_append_basic(&reply_args, DBUS_TYPE_STRING, &reply_str))
	{
		Logger("Error in dbus_message_iter_append_basic()\n");
		return EXIT_FAILURE;
	}
	if (!dbus_connection_send(conn, reply, NULL))
	{
		Logger("Error in dbus_connection_send()\n");
		return EXIT_FAILURE;
	}
	dbus_connection_flush(conn);
	dbus_message_unref(reply);
	return EXIT_SUCCESS;
}

int DBus_reply_msg_with_error(DBusConnection *conn, DBusMessage *msg, char *reply_str)
{
	DBusMessage *dbus_error_msg;
	if ((dbus_error_msg = dbus_message_new_error(msg, DBUS_ERROR_FAILED, reply_str)) == NULL)
	{
		Logger("Error in dbus_message_new_error()\n");
		return EXIT_FAILURE;
	}
	if (!dbus_connection_send(conn, dbus_error_msg, NULL)) {
		Logger("Error in dbus_connection_send()\n");
		return EXIT_FAILURE;
	}
	dbus_connection_flush(conn);
	dbus_message_unref(dbus_error_msg);
	return EXIT_SUCCESS;
}

unsigned char get_rmswORmux_sw_name(unsigned char tele_type, char *buf)//String Decoder for sw_name. Return sw_name's enum on success. 0xff otherwise.
{
	switch(tele_type)
	{
		case MUX:
			if(!strcmp(buf,"Sel_1"))
				return Sel_1;
			else if(!strcmp(buf,"Sel_2"))
				return Sel_2;
			else if(!strcmp(buf,"Sel_3"))
				return Sel_3;
			else if(!strcmp(buf,"Sel_4"))
				return Sel_4;
			break;
		case RMSW_2CH:
			if(!strcmp(buf,"Main_SW"))
				return Main_SW;
			else if(!strcmp(buf,"SW_1"))
				return SW_1;
			else if(!strcmp(buf,"SW_2"))
				return SW_2;
			break;
		case Mini_RMSW:
			if(!strcmp(buf,"Main_SW"))
				return Main_SW;
			break;
		default : return -1;
	}
	return -1;
}

char * new_MTI_config_argValidator(cJSON *JSON_args, unsigned char *new_RF_CH,unsigned char *new_mode, union MTI_specific_regs *sregs)
{
	char *buf;

	if(!JSON_args||!new_RF_CH||!new_mode||!sregs)
		return "NULL at Argument(s)";

	//Validate data in argument
	if(!cJSON_HasObjectItem(JSON_args,"new_RF_CH") || !cJSON_HasObjectItem(JSON_args,"new_mode"))
		return "new_MTI_config(): Method's Arguments are Invalid";
	if(cJSON_GetObjectItem(JSON_args,"new_RF_CH")->type != cJSON_Number)
		return "new_MTI_config(): new_RF_CH in NAN";
	*new_RF_CH = cJSON_GetObjectItem(JSON_args,"new_RF_CH")->valueint;
	if(cJSON_GetObjectItem(JSON_args,"new_mode")->type != cJSON_String)//Check if new_mode is string
		return "new_MTI_config(): new_mode is NOT a string";
	buf = cJSON_GetObjectItem(JSON_args,"new_mode")->valuestring;
	for(*new_mode=0; MTI_Tele_dev_type_str[*new_mode]&&strcmp(buf, MTI_Tele_dev_type_str[*new_mode]); *new_mode+=1)
		;
	if(*new_mode>Dev_type_max)//Check if new_mode is valid
		return "new_MTI_config(): new_mode is Unknown";
	if(*new_mode != RM_SW_MUX && *new_mode != Tele_quad)
	{
		sregs->for_temp_tele.StV = cJSON_HasObjectItem(JSON_args,"StV") ? cJSON_GetObjectItem(JSON_args,"StV")->valueint:0;
		sregs->for_temp_tele.StF = cJSON_HasObjectItem(JSON_args,"StF") ? cJSON_GetObjectItem(JSON_args,"StF")->valueint:0;
	}
	else if(*new_mode == RM_SW_MUX)
	{
		if(cJSON_HasObjectItem(JSON_args,"G_SW") && cJSON_HasObjectItem(JSON_args,"G_SL"))
		{
			sregs->for_rmsw_dev.manual_button = cJSON_GetObjectItem(JSON_args,"G_SW")->valueint?1:0;
			sregs->for_rmsw_dev.sleep_button = cJSON_GetObjectItem(JSON_args,"G_SL")->valueint?1:0;
		}
		else
			return "new_MTI_config(): Missing Arguments";
	}
	return NULL;
}

char * ctrl_tele_SWs_argValidator(cJSON *JSON_args, unsigned char *mem_pos, unsigned char *tele_type, unsigned char *sw_name)
{
	char *buf;

	if(!JSON_args||!mem_pos||!tele_type||!sw_name)
		return "NULL at Argument(s)";

	if(!cJSON_HasObjectItem(JSON_args,"mem_pos")|| !cJSON_HasObjectItem(JSON_args,"tele_type")||
	   !cJSON_HasObjectItem(JSON_args,"sw_name")|| !cJSON_HasObjectItem(JSON_args,"new_state"))
		return "ctrl_tele_SWs(): Missing Arguments";
	if(cJSON_GetObjectItem(JSON_args,"mem_pos")->type != cJSON_Number)
		return "ctrl_tele_SWs(): mem_pos is NAN";
	*mem_pos = cJSON_GetObjectItem(JSON_args,"mem_pos")->valueint;
	if(cJSON_GetObjectItem(JSON_args,"tele_type")->type != cJSON_String)
		return "ctrl_tele_SWs(): tele_type isn't string";
	buf = cJSON_GetObjectItem(JSON_args,"tele_typePWM_gens_config")->valuestring;
	for(*tele_type=0; MTI_RM_dev_type_str[*tele_type]&&strcmp(buf, MTI_RM_dev_type_str[*tele_type]); *tele_type+=1)
		;
	if(*tele_type>Tele_type_max)//Check if tele_type is valid
		return "new_MTI_config(): tele_type is Unknown";
	if(cJSON_GetObjectItem(JSON_args,"sw_name")->type != cJSON_String)
		return "ctrl_tele_SWs(): sw_name isn't string";
	buf = cJSON_GetObjectItem(JSON_args,"sw_name")->valuestring;
	if((*sw_name = get_rmswORmux_sw_name(*tele_type, buf)) == 0xff)
		return "ctrl_tele_SWs(): sw_name is Unknown";
	return NULL;
}

char * new_PWM_config_argValidator(cJSON *JSON_args, struct Gen_config_struct PWM_gens_config[])
{
	static char ret_str[100];
	cJSON *JSON_element, *JSON_Array;

	if(!JSON_args||!PWM_gens_config)
		return "NULL at Argument(s)";
	if(!cJSON_HasObjectItem(JSON_args,"PWM_gens_config"))
		return "new_PWM_config(): PWM_gens_config Missing!!!";
	if(cJSON_GetObjectItem(JSON_args,"PWM_gens_config")->type != cJSON_Array)
		return "new_PWM_config(): PWM_gens_config isn't Array";
	JSON_Array = cJSON_GetObjectItem(JSON_args,"PWM_gens_config");
	if(cJSON_GetArraySize(JSON_Array)!=2)
		return "new_PWM_config(): PWM_gens_config wrong size Array";
	for(int i=0; (JSON_element = cJSON_GetArrayItem(JSON_Array, i)) && i<2; i++)
	{
		if(!cJSON_HasObjectItem(JSON_element,"max") || !cJSON_HasObjectItem(JSON_element,"min") || !cJSON_HasObjectItem(JSON_element,"saturation"))
			return "new_PWM_config(): Missing Arguments";
		if(cJSON_GetObjectItem(JSON_element,"max")->type != cJSON_Number)
		{
			sprintf(ret_str, "new_PWM_config(): PWM_gens_config[%u].max is NAN", i);
			return ret_str;
		}
		if(cJSON_GetObjectItem(JSON_element,"min")->type != cJSON_Number)
		{
			sprintf(ret_str, "new_PWM_config(): PWM_gens_config[%u].min is NAN", i);
			return ret_str;
		}
		if(cJSON_GetObjectItem(JSON_element,"saturation")->type != cJSON_False &&
		   cJSON_GetObjectItem(JSON_element,"saturation")->type != cJSON_True )
		{
			sprintf(ret_str, "new_PWM_config(): PWM_gens_config[%u].saturation isn't Boolean", i);
			return ret_str;
		}
		PWM_gens_config[i].max = cJSON_GetObjectItem(JSON_element,"max")->valueint;
		PWM_gens_config[i].min = cJSON_GetObjectItem(JSON_element,"min")->valueint;
		PWM_gens_config[i].pwm_mode.dec.saturation = cJSON_GetObjectItem(JSON_element,"saturation")->valueint;
	}
	return NULL;
}

/*
struct Gen_config_struct{
	unsigned int max;
	unsigned int min;
	union generator_mode{
		struct decoder_for_generator_mode{
			unsigned saturation:1;
			unsigned reserved:6;
			unsigned fixed_freq:1;
		}dec;
		unsigned char as_byte;
	}pwm_mode;
};
*/


