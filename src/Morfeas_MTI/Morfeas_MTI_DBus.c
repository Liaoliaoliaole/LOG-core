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

	//--- MTI's Write Functions ---//
//MTI function that sending a new Radio configuration. Return 0 on success, errno otherwise.
int set_MTI_Radio_config(modbus_t *ctx, unsigned char new_RF_CH, unsigned char new_mode, union MTI_specific_regs *new_sregs);
//MTI function that write the Global power switch. Return 0 on success, errno otherwise.
int set_MTI_Global_switches(modbus_t *ctx, bool global_power);
//MTI function that write a new configuration for PWM generators, Return 0 on success, errno otherwise.
int set_MTI_PWM_gens(modbus_t *ctx, struct Gen_config_struct *new_Config);
//MTI function that controlling the state of a controllable telemetry(RMSW, MUX, Mini), Return 0 on success, errno otherwise.
int ctrl_tele_switch(modbus_t *ctx, unsigned char mem_pos, unsigned char dev_type, unsigned char sw_name, bool new_state);

//static const char *const OBJECT_PATH_NAME = "/Morfeas/MTI/DBUS_server_app";
static const char *const Method[] = {"new_MTI_config", "MTI_Global_SWs", "new_PWM_config", "ctrl_tele_SWs", "echo"};
static DBusError dbus_error;

//Local DBus Error Logging function
void Log_DBus_error(char *str);
int DBus_reply_msg(DBusConnection *conn, DBusMessage *msg, char *reply_str);
int DBus_reply_msg_with_error(DBusConnection *conn, DBusMessage *msg, char *reply_str);

//D-Bus listener function
void * MTI_DBus_listener(void *varg_pt)//Thread function.
{
	//Decoded variables from passer
	modbus_t *ctx = *(((struct MTI_DBus_thread_arguments_passer *)varg_pt)->ctx);
	struct Morfeas_MTI_if_stats *stats = ((struct MTI_DBus_thread_arguments_passer *)varg_pt)->stats;
	//D-Bus related variables
	char *dbus_server_name_if;
	int ret;
	DBusConnection *conn;
	DBusMessage *msg;
	/*
	//Local variables and structures
	union MTI_specific_regs sregs;
	struct Gen_config_struct PWM_gens_config[2];
	*/
	
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
	
    ret = dbus_bus_request_name(conn, dbus_server_name_if, DBUS_NAME_FLAG_DO_NOT_QUEUE, &dbus_error);
    if(dbus_error_is_set (&dbus_error))
        Log_DBus_error("dbus_bus_request_name() Failed!!!");

    if(ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        Logger("Dbus: not primary owner, ret = %d\n", ret);
        return NULL;
    }
	//free dbus_server_name_if
	free(dbus_server_name_if);
	
	//Allocate space and create dbus_server_if
	if(!(dbus_server_name_if = calloc(sizeof(char), strlen(IF_NAME_PROTO)+strlen(stats->dev_name)+1)))
	{
		fprintf(stderr,"Memory error!!!\n");
		exit(EXIT_FAILURE);
	}
	sprintf(dbus_server_name_if, "%s%s", IF_NAME_PROTO, stats->dev_name);
	Logger("\t Interface:\"%s\"\n", dbus_server_name_if);

	// Handle request from clients
	while(handler_run)
	{
		//Wait for incoming messages, timeout in 1 sec
        if(dbus_connection_read_write_dispatch(conn, 1000))
		{
			if((msg = dbus_connection_pop_message(conn)))
			{
				//Analyze received message for methods call
				if(dbus_message_is_method_call(msg, dbus_server_name_if, Method[4]))//Check for "echo" call
				{
					DBus_reply_msg(conn, msg, "Reply to test_method call!!!\n");
				}
			}
		}
	}

	dbus_error_free(&dbus_error);
	free(dbus_server_name_if);
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
	DBusMessageIter args;
	//Send reply
	if(!(reply = dbus_message_new_method_return(msg)))
	{
		Logger("Error in dbus_message_new_method_return()\n");
		return EXIT_FAILURE;
	}
	
	dbus_message_iter_init_append(reply, &args);

	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &reply_str))
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
	if ((dbus_error_msg = dbus_message_new_error (msg, DBUS_ERROR_FAILED, reply_str)) == NULL)
	{
		Logger("Error in dbus_message_new_error()\n");
		return EXIT_FAILURE;
	}
	if (!dbus_connection_send (conn, dbus_error_msg, NULL)) {
		Logger("Error in dbus_connection_send()\n");
		return EXIT_FAILURE;
	}
	dbus_connection_flush (conn);
	dbus_message_unref (dbus_error_msg);
	return EXIT_SUCCESS;
}
