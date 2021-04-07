/*
File: Morfeas_NOX_ws.c, Implementation of WebSocket functionality for Morfeas_NOX_if, Part of Morfeas_project.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>

#include <nopoll.h>

#include "../Morfeas_Types.h"
#include "../Supplementary/Morfeas_Logger.h"

//External variables
extern volatile unsigned char NOX_handler_run;
extern pthread_mutex_t NOX_access;
//Global static variables
static noPollCtx *ctx = NULL;

//Callback function
static void listener_on_message(noPollCtx *ctx, noPollConn *conn, noPollMsg *msg, noPollPtr user_data);

//WebSocket listener Thread function
void * Morfeas_NOX_ws_server(void *varg_pt)
{
	struct Morfeas_NOX_if_stats *stats;
	int i, error_code, port = 8081;
	char p_buff[10];
	noPollConn *listener;

	if(ctx || !varg_pt)//Immediately return if ctx is set or if varg_pt is NULL.
		return NULL;
	stats = ((struct NOX_DBus_thread_arguments_passer *)varg_pt)->stats;//Decoded variables from passer
	ctx = nopoll_ctx_new();
	//nopoll_log_enable(ctx, nopoll_true);
	for(i=0; i<4; i++)
	{
		sprintf(p_buff, "%hu", port+i);
		if(nopoll_conn_is_ok((listener = nopoll_listener_new(ctx, "0.0.0.0", p_buff))))
			break;
	}
	if(i>=4)
	{
		Logger("ERROR: nopoll_listener_new() Failed, WebSocket server will terminate\n");
		return NULL;
	}
	//Set ws_port with server's TCP port.
	pthread_mutex_lock(&NOX_access);
		stats->ws_port = port;
	pthread_mutex_unlock(&NOX_access);

	Logger("WebSocket server started and listening@%s\n", nopoll_conn_port(listener));//, nopoll_conn_ref_count(listener));
	//Set Callback functions
	nopoll_ctx_set_on_msg (ctx, listener_on_message, NULL);
	//Server's main loop
	while(NOX_handler_run)
	{
		error_code = nopoll_loop_wait(ctx, 0);//Process WebSocket events
		if(error_code == -4)
		{
			 Logger("Log here you had an error cause by the io waiting mechanism, errno=%d\n", errno);
			 // recover by just calling io wait engine
			 // try to limit recoveries to avoid infinite loop
		}
	}
	nopoll_conn_close(listener);//unref connection
	Logger("Listener: finishing references: %d\n", nopoll_ctx_ref_count(ctx));
	//Clean up
	nopoll_ctx_unref(ctx);
	nopoll_cleanup_library();
	ctx = NULL;
	return NULL;
}

void Morfeas_NOX_ws_server_send_meas()
{

}

void Morfeas_NOX_ws_server_stop()
{
	nopoll_loop_stop(ctx);
}

static void listener_on_message(noPollCtx * ctx, noPollConn * conn, noPollMsg * msg, noPollPtr  user_data)
{
	//nopoll_conn_send_text(conn, "Message received", 16);
	return;
}