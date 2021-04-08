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
#define PORT_init 8081
#define PORT_pool_size 4
#define MAX_AMOUNT_OF_CLIENTS 2

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

struct WS_send_frame_struct{
	float NOx_val[2];
	float O2_val[2];
};
struct WS_NOX_sensors_data_struct{
	unsigned char update;
	struct WS_send_frame_struct WS_send_frame;
}static WS_NOX_sensors_data={0};

//External variables
extern volatile unsigned char NOX_handler_run;
extern pthread_mutex_t NOX_access;

//Global static variables
static volatile unsigned char amount_of_clients;
static noPollCtx *master_ctx = NULL;

//Callback function
static nopoll_bool Morfeas_NOX_ws_server_on_open(noPollCtx *ctx, noPollConn *conn, noPollPtr user_data);
static void Morfeas_NOX_ws_server_on_close(noPollCtx * ctx, noPollConn * conn, noPollPtr user_data);
static void Morfeas_NOX_ws_server_on_msg(noPollCtx * ctx, noPollConn * conn, noPollMsg * msg, noPollPtr  user_data);

static nopoll_bool WS_NOX_sensors_data_send(noPollCtx *ctx, noPollConn *conn, noPollPtr user_data);

//WebSocket listener Thread function
void * Morfeas_NOX_ws_server(void *varg_pt)
{
	struct Morfeas_NOX_if_stats *stats;
	int port;
	char p_buff[10];
	noPollConn *WS_serv = NULL;

	if(master_ctx || !varg_pt)//Immediately return if master_ctx is set or if varg_pt is NULL.
		return NULL;
	stats = ((struct NOX_DBus_thread_arguments_passer *)varg_pt)->stats;//Decoded variables from passer
	master_ctx = nopoll_ctx_new();
	amount_of_clients = 0;
	//nopoll_log_enable(master_ctx, nopoll_true);
	for(port = PORT_init; port<(PORT_init+PORT_pool_size); port++)
	{
		sprintf(p_buff, "%hu", port);
		if(nopoll_conn_is_ok((WS_serv = nopoll_listener_new(master_ctx, "0.0.0.0", p_buff))))
			break;
		usleep(100000);
	}
	if(port>=(PORT_init+PORT_pool_size))
	{
		Logger("ERROR: nopoll_listener_new() Failed, WebSocket server will terminate\n");
		return NULL;
	}
	//Set ws_port with server's TCP port.
	pthread_mutex_lock(&NOX_access);
		stats->ws_port = port;
	pthread_mutex_unlock(&NOX_access);

	Logger("WebSocket server started and listening@%s\n", nopoll_conn_port(WS_serv));//, nopoll_conn_ref_count(listener));
	//Set Callback functions
	nopoll_ctx_set_on_open(master_ctx, Morfeas_NOX_ws_server_on_open, NULL);
	nopoll_ctx_set_on_msg(master_ctx, Morfeas_NOX_ws_server_on_msg, NULL);
	//Server's main loop
	while(NOX_handler_run)
	{
		nopoll_loop_wait(master_ctx, 5000);//Process WebSocket events
	}
	nopoll_conn_close(WS_serv);
	Logger("Listener: finishing references: %d\n", nopoll_ctx_ref_count(master_ctx));
	//Clean up
	nopoll_ctx_unref(master_ctx);
	nopoll_cleanup_library();
	master_ctx = NULL;
	return NULL;
}

void Morfeas_NOX_ws_server_send_meas(struct UniNOx_sensor *NOXs_data)
{
	if(!NOXs_data || !master_ctx)
		return;
	Logger("nopoll_ctx_foreach_conn()\n");
	nopoll_ctx_foreach_conn(master_ctx, WS_NOX_sensors_data_send, &(WS_NOX_sensors_data.WS_send_frame));
}

static nopoll_bool Morfeas_NOX_ws_server_on_open(noPollCtx *ctx, noPollConn *conn, noPollPtr user_data)
{
	if(amount_of_clients>=MAX_AMOUNT_OF_CLIENTS)
	{
		Logger("Max amount of Clients is reached!!!\n");
		return nopoll_false;
	}
	nopoll_conn_set_on_close(conn, Morfeas_NOX_ws_server_on_close, NULL);
	Logger("New Connection from %s\n", nopoll_conn_host(conn));
	amount_of_clients++;
	return nopoll_true;
}

static void Morfeas_NOX_ws_server_on_close(noPollCtx * ctx, noPollConn * conn, noPollPtr user_data)
{
	amount_of_clients--;
	Logger("Connection close from %s\n", nopoll_conn_host(conn));
	return;
}

static void Morfeas_NOX_ws_server_on_msg(noPollCtx * ctx, noPollConn * conn, noPollMsg * msg, noPollPtr  user_data)
{
	// reply to the message
	Logger("Received msg: %s\n", nopoll_msg_get_payload(msg));
	nopoll_conn_send_text(conn, "Message received", 16);
	return;
}

static nopoll_bool WS_NOX_sensors_data_send(noPollCtx *ctx, noPollConn *conn, noPollPtr user_data)
{
	struct WS_send_frame_struct *WS_send_frame = (struct WS_send_frame_struct*)user_data;
	Logger("call\n");
	nopoll_conn_send_text(conn, "Message TX", 10);
	return nopoll_true;
}