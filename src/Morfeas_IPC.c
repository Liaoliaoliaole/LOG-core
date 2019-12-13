/*
File: Morfeas_IPC.h, Implementation of functions for IPC.
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
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Morfeas_IPC.h"

size_t Morfeas_IPC_msg_size[]={
	sizeof(Handler_reg_op_msg),
	sizeof(Handler_reg_op_msg),
	sizeof(SDAQ_reg_update_msg),
	sizeof(SDAQ_clear_msg),
	sizeof(SDAQ_info_msg),
	sizeof(SDAQ_timediff_msg),
	sizeof(SDAQ_meas_msg),
	sizeof(CAN_BUS_info_msg)
};

char *Morfeas_IPC_handler_type_name[]={
	"SDAQ","MDAQ","IO-BOX","MTI"
};

	//----TX Functions----//
//function for TX, return the amount of bytes that transmitted through the FIFO, or 0 in failure
size_t IPC_msg_TX(int FIFO_fd, IPC_message *IPC_msg_ptr)//const char *path_to_FIFO,
{
	fd_set writeCheck;
    fd_set errCheck;
    struct timeval timeout;
	int select_ret;
	ssize_t writen_bytes = 0;

	FD_ZERO(&writeCheck);
    FD_ZERO(&errCheck);
	FD_SET(FIFO_fd, &writeCheck);
	FD_SET(FIFO_fd, &errCheck);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	select_ret = select(FIFO_fd+1, NULL, &writeCheck, &errCheck, &timeout);
	if (select_ret < 0)
		perror("TX -> Select failed ");
	else if (FD_ISSET(FIFO_fd, &errCheck))
		perror("TX -> FD error ");
	else if (FD_ISSET(FIFO_fd, &writeCheck))
		writen_bytes = write(FIFO_fd, IPC_msg_ptr, sizeof(IPC_message));
	return writen_bytes;
}
//Function for construction of message for registration of a Handler
size_t IPC_Handler_reg_op(int FIFO_fd, unsigned char handler_type, char *connected_to_BUS, unsigned char unreg)//const char *path_to_FIFO,
{
	IPC_message IPC_reg_msg;
	//Construct and send Handler registration msg
	IPC_reg_msg.Handler_reg.IPC_msg_type = unreg ? IPC_Handler_unregister : IPC_Handler_register;
	IPC_reg_msg.Handler_reg.handler_type = handler_type;
	memccpy(&(IPC_reg_msg.Handler_reg.connected_to_BUS), connected_to_BUS, '\0', 10);
	IPC_reg_msg.Handler_reg.connected_to_BUS[9] = '\0';
	return IPC_msg_TX(FIFO_fd, &IPC_reg_msg);
}
	//----RX Function----//
//function for RX, return the type of the received message or 0 in failure
unsigned char IPC_msg_RX(int FIFO_fd, IPC_message *IPC_msg_ptr)
{
	fd_set readCheck;
    fd_set errCheck;
    struct timeval timeout;
	unsigned char type;
	int select_ret;
	ssize_t read_bytes = -1;
	FD_ZERO(&readCheck);
    FD_ZERO(&errCheck);
	FD_SET(FIFO_fd, &readCheck);
	FD_SET(FIFO_fd, &errCheck);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	select_ret = select(FIFO_fd+1, &readCheck, NULL, &errCheck, &timeout);
	if (select_ret < 0)
		perror("RX -> Select failed ");
	else if (FD_ISSET(FIFO_fd, &errCheck))
		perror("RX -> FD error ");
	else if (FD_ISSET(FIFO_fd, &readCheck))
	{
		read_bytes = read(FIFO_fd, IPC_msg_ptr, sizeof(IPC_message));
		if((type = ((unsigned char *)IPC_msg_ptr)[0]) <= Morfeas_IPC_MAX_type)
			return type;
	}
	if(read_bytes != -1)
		printf("Wrong amount of Bytes!!!\n");
	return 0;
}