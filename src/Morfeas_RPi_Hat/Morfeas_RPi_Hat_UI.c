/*
Program: Morfeas_RPi_Hat_if.c  Implementation of supporting software for Morfeas_RPi_Hat.
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

/* ncurses windows sizes definitions*/
#define w_csa_stat_info_height 10
#define w_csa_stat_info_width 25
#define w_spacing 0
#define w_meas_width  w_stat_info_width
#define term_min_width  w_csa_stat_info_width*4 + w_spacing
#define term_min_height  w_csa_stat_info_height + 20

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <ncurses.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../Supplementary/Morfeas_run_check.h"
#include "Morfeas_RPi_Hat.h"

//Structs def
struct windows_init_arg
{
	int det_ports;
	WINDOW *Port_csa[4], *UI_term;
};

//Global variables
pthread_mutex_t ncurses_access = PTHREAD_MUTEX_INITIALIZER;
volatile unsigned char running = 1;
struct Morfeas_RPi_Hat_EEPROM_SDAQnet_Port_config Ports_config[4] = {0};
struct Morfeas_RPi_Hat_Port_meas Ports_meas[4] = {0};

//Local Functions
void sigint_signal_handler(int signum)
{
	running = 0;
	return;
}
void w_init(struct windows_init_arg *arg);


int main(int argc, char *argv[])
{
	int i;
	//Variables for ncurses
	struct windows_init_arg win_arg;
	struct winsize term_init_size;

	//Check if program already runs.
	if(check_already_run(argv[0]))
	{
		fprintf(stderr, "%s Already Running!!!\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	//Check if Morfeas_SDAQ_if running.
	if(check_already_run("Morfeas_SDAQ_if"))
	{
		fprintf(stderr, "Morfeas_SDAQ_if detected to run, %s can't run!!!\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	ioctl(STDOUT_FILENO, TIOCGWINSZ, &term_init_size);// get current size of terminal window
	//Check if the terminal have the minimum size for the application
	if(term_init_size.ws_col<term_min_width || term_init_size.ws_row<term_min_height)
	{
		printf("Terminal need to be at least %dX%d Characters\n",term_min_width,term_min_height);
		return EXIT_SUCCESS;
	}

	//Link signal SIGINT to quit_signal_handler
	signal(SIGINT, sigint_signal_handler);

	//Detect amount of CSAs - TODO
	win_arg.det_ports=2;

	//Start ncurses
	initscr(); // start the ncurses mode
	curs_set(0);//hide cursor
	noecho();//disable echo
	//raw();//getch without return
	keypad(stdscr, TRUE);
	scrollok(stdscr, TRUE);
	w_init(&win_arg);//Init windows

	//Main loop:Read values from Port's SCAs (MAX9611)
	while(running)
	{
		for(i=0; i<win_arg.det_ports; i++)
		{
			mvwprintw(win_arg.Port_csa[i],1,1, "Port %u", i);
			if(!get_port_meas(&Ports_meas[i], i, I2C_BUS_NUM))
			{
				mvwprintw(win_arg.Port_csa[i],2,2, "voltage= %.1fV", Ports_meas[i].port_voltage*MAX9611_volt_meas_scaler);
				mvwprintw(win_arg.Port_csa[i],3,2, "current= %.3fA", (Ports_meas[i].port_current)*0.001222);
				mvwprintw(win_arg.Port_csa[i],4,2, "Temperature= %.1f°C", Ports_meas[i].temperature*MAX9611_temp_scaler);
			}
			else
				mvwprintw(win_arg.Port_csa[i],2,2, "Error !!!");
			wrefresh(win_arg.Port_csa[i]);
		}
		/*
		get_port_meas(&Ports_meas[0], 0, I2C_BUS_NUM);
		printf("\nPort0_voltage= %.1fV\n",Ports_meas[0].port_voltage*MAX9611_volt_meas_scaler);
		printf("Port0_current= %.3fA\n",(Ports_meas[0].port_current)*0.001222);
		printf("Die0_temperature= %.0f°C\n",Ports_meas[0].temperature*MAX9611_temp_scaler);

		get_port_meas(&Ports_meas[1], 1, I2C_BUS_NUM);
		printf("\nPort1_voltage= %.1fV\n",Ports_meas[1].port_voltage*MAX9611_volt_meas_scaler);
		printf("Port1_current= %.3fA\n",(Ports_meas[1].port_current)*0.001222);
		printf("Die1_temperature= %.0f°C\n",Ports_meas[1].temperature*MAX9611_temp_scaler);
		*/
		usleep(100000);
	}

	endwin();

	return EXIT_SUCCESS;
}


void wclean_refresh(WINDOW *ptr)
{
	wclear(ptr);
	box(ptr,0,0);
	wrefresh(ptr);
	return;
}

void w_init(struct windows_init_arg *arg)
{
	int term_col,term_row, offset, i;
	getmaxyx(stdscr,term_row,term_col);
	offset = (term_col - w_csa_stat_info_width*arg->det_ports)/(2*arg->det_ports);
	//Create windows for CSA measurements
	for(i=0; i < arg->det_ports; i++)
	{
		arg->Port_csa[i] = newwin(w_csa_stat_info_height, w_csa_stat_info_width, 0, i*term_col/arg->det_ports+w_spacing+offset);
		scrollok(arg->Port_csa[i], TRUE);
	}
	arg->UI_term = newwin(term_row - w_csa_stat_info_height, term_col, w_csa_stat_info_height, 0);
	clear();
	refresh();
	for(i=0; i < arg->det_ports; i++)
		wclean_refresh(arg->Port_csa[i]);
	wclean_refresh(arg->UI_term);
	return;
}


/*
//Struct for EEPROM(24AA08) data
struct Morfeas_RPi_Hat_EEPROM_SDAQnet_Port_config{
	struct last_port_calibration_date{
		unsigned char year;
		unsigned char month;
		unsigned char day;
	}last_cal_date;
	char volt_meas_offset;
	char curr_meas_offset;
	float curr_meas_scaler;
	unsigned char checksum;
};


	Ports_config[0].last_cal_date.year = 1;
	Ports_config[0].last_cal_date.month = 3;
	Ports_config[0].last_cal_date.day = 5;
	Ports_config[0].volt_meas_offset = 0;
	Ports_config[0].curr_meas_offset = 0;
	Ports_config[0].curr_meas_scaler = 0.001222;
	if(write_port_config(&Ports_config[0], 0, I2C_BUS_NUM))
		printf("%s",Morfeas_hat_error());

	if(write_port_config(&Ports_config[0], 1, I2C_BUS_NUM))
		printf("%s",Morfeas_hat_error());

	if(erase_EEPROM(0, I2C_BUS_NUM))
		printf("%s",Morfeas_hat_error());

	//Get EEPROM config data
	for(int i=0;i<4;i++)
	{
		if(read_port_config(&Ports_config[i], i, I2C_BUS_NUM))
			printf("%s",Morfeas_hat_error());
	}
	MAX9611_init(1,1);
	while(running)
	{
		get_port_meas(&Ports_meas[0], 0, I2C_BUS_NUM);
		printf("\nPort0_voltage= %.1fV\n",Ports_meas[0].port_voltage*MAX9611_volt_meas_scaler);
		printf("Port0_current= %.3fA\n",(Ports_meas[0].port_current)*0.001222);
		printf("Die0_temperature= %.0f°C\n",Ports_meas[0].temperature*MAX9611_temp_scaler);

		get_port_meas(&Ports_meas[1], 1, I2C_BUS_NUM);
		printf("\nPort1_voltage= %.1fV\n",Ports_meas[1].port_voltage*MAX9611_volt_meas_scaler);
		printf("Port1_current= %.3fA\n",(Ports_meas[1].port_current)*0.001222);
		printf("Die1_temperature= %.0f°C\n",Ports_meas[1].temperature*MAX9611_temp_scaler);

		sleep(1);
	}
	*/
