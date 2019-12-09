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
	sizeof(SDAQ_register_msg),
	sizeof(SDAQ_clean_msg),
	sizeof(SDAQ_info_msg),
	sizeof(SDAQ_timediff_msg),
	sizeof(SDAQ_meas_msg)
};	

	//----RX/TX Functions----//
//function for TX, return the amount of bytes that transmitted through the FIFO, or 0 in failure
int IPC_msg_TX(char *path_to_FIFO, IPC_msg *IPC_msg_ptr, unsigned char type)
{
	fd_set writeCheck;
    fd_set errCheck;
    struct timeval timeout;
	int FIFO_fd, select_ret;
	ssize_t writen_bytes = 0;
	
	if(access(path_to_FIFO, F_OK) == -1 )//Make the Named Pipe(FIFO) if is not exist
	{
		mkfifo(path_to_FIFO, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
		return 0;
	}
	FD_ZERO(&writeCheck);
    FD_ZERO(&errCheck);
	FIFO_fd = open(path_to_FIFO, O_RDWR );//O_NONBLOCK
	FD_SET(FIFO_fd, &writeCheck);
	FD_SET(FIFO_fd, &errCheck);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	select_ret = select(FIFO_fd+1, NULL, &writeCheck, &errCheck, &timeout);
	if (select_ret < 0)
		perror("Select failed!!!");
	else if (FD_ISSET(FIFO_fd, &errCheck))
		perror("FD error!!!");
	else if (FD_ISSET(FIFO_fd, &writeCheck))
	{
		if(type)
		{
			write(FIFO_fd, &type, sizeof(unsigned char));
			writen_bytes = Morfeas_IPC_msg_size[type - 1];
			writen_bytes = write(FIFO_fd, IPC_msg_ptr, writen_bytes);
		}
	}
	else
		printf("Timeout!!!\n");
	close(FIFO_fd);
	return writen_bytes;
}
//function for RX, return the type of the received message or 0 in failure
int IPC_msg_RX(char *path_to_FIFO, IPC_msg *IPC_msg_ptr)
{
	fd_set readCheck;
    fd_set errCheck;
    struct timeval timeout;
	unsigned char type; 
	int FIFO_fd, select_ret;
	ssize_t read_bytes = -1;
	
	if(access(path_to_FIFO, F_OK) == -1 )//Make the Named Pipe(FIFO) if is not exist
	{
		mkfifo(path_to_FIFO, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
		return 0;
	}
	FD_ZERO(&readCheck);
    FD_ZERO(&errCheck);
	FIFO_fd = open(path_to_FIFO, O_RDWR );//O_NONBLOCK
	FD_SET(FIFO_fd, &readCheck);
	FD_SET(FIFO_fd, &errCheck);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	select_ret = select(FIFO_fd+1, &readCheck, NULL, &errCheck, &timeout);
	if (select_ret < 0)
		perror("Select failed!!!");
	else if (FD_ISSET(FIFO_fd, &errCheck))
		perror("FD error!!!");
	else if (FD_ISSET(FIFO_fd, &readCheck))
	{
		read(FIFO_fd, &type, sizeof(unsigned char));
		if(type)
		{
			read_bytes = Morfeas_IPC_msg_size[type - 1];
			printf("Type = %d\nRead_bytes=%d\n",type,read_bytes);
			read_bytes -= read(FIFO_fd, IPC_msg_ptr, read_bytes);
		}
	}
	else
		printf("Timeout!!!\n");
	close(FIFO_fd);
	if(!read_bytes)
		return type;
	else
		return 0;	
}