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
/* Shell command buffer */
#define user_inp_buf_size 80
#define max_amount_of_user_arg 20
#define history_buffs_length 30

/* ncurses windows sizes definitions */
#define w_csa_stat_info_height 8
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
#include <glib.h>
#include <gmodule.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../Supplementary/Morfeas_run_check.h"
#include "Morfeas_RPi_Hat.h"

//Structs def
struct windows_init_arg{
	int det_ports;
	WINDOW *Port_csa[4], *UI_term;
};
typedef struct{
	char usr_in_buff[user_inp_buf_size];
}history_buffer_entry;

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

//slice free function for history_buffs_nodes
void history_buff_free_node(gpointer node)
{
	g_slice_free(history_buffer_entry, node);
}

//Function for initialiazation of ncurses windows
void w_init(struct windows_init_arg *arg);
//Function for decoding printing of last the last calibration date of a Port Surrent sense amplifier. 
char *last_cal_print(struct last_port_calibration_date);

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

	//Init and Detect amount of CSAs and get config for each
	win_arg.det_ports=0;
	for(i=0;i<4;i++)
	{
		if(!MAX9611_init(i,I2C_BUS_NUM))
		{
			win_arg.det_ports++;
			if(read_port_config(&Ports_config[i], i, I2C_BUS_NUM))
				Ports_config[i].curr_meas_scaler = 0.001222; //Default current scaler (for 22 mohm shunt)
		}
	}
	//Exit if no SCA detected
	if(!win_arg.det_ports)
	{
		fprintf(stderr, "No Port Detected!!!\n");
		exit(EXIT_FAILURE);
	}
	
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
		curs_set(0);
		for(i=0; i<win_arg.det_ports; i++)
		{
			mvwprintw(win_arg.Port_csa[i],1,6, "Port %u (can%u)", i, i);
			if(!get_port_meas(&Ports_meas[i], i, I2C_BUS_NUM))
			{
				mvwprintw(win_arg.Port_csa[i],2,2, "Last_Cal = %s", last_cal_print(Ports_config[i].last_cal_date));
				mvwprintw(win_arg.Port_csa[i],3,2, "Voltage = %.1fV", (Ports_meas[i].port_voltage - Ports_config[i].volt_meas_offset)*MAX9611_volt_meas_scaler);
				mvwprintw(win_arg.Port_csa[i],4,2, "Current = %.3fA", (Ports_meas[i].port_current - Ports_config[i].curr_meas_offset)*Ports_config[i].curr_meas_scaler);
				mvwprintw(win_arg.Port_csa[i],5,2, "Shunt_Temp = %.1f°C", Ports_meas[i].temperature*MAX9611_temp_scaler);
			}
			else
				mvwprintw(win_arg.Port_csa[i],2,2, "Error !!!");
			wrefresh(win_arg.Port_csa[i]);
		}
		curs_set(1);
		usleep(100000);
	}

	endwin();

	return EXIT_SUCCESS;
}

char *last_cal_print(struct last_port_calibration_date last_date)
{
	static char buff[10];
	struct tm last_cal_tm = {0};
	
	if(!last_date.year &&
	   !last_date.month &&
	   !last_date.day)
	   return "UnCal";
	   
	last_cal_tm.tm_year = last_date.year - 100;
	last_cal_tm.tm_mon = last_date.month - 1;
	last_cal_tm.tm_mday = last_date.day;
	strftime(buff, sizeof(buff), "%x", &last_cal_tm);
	return buff;
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

//Implementation of the user's Interface function
void shell(WINDOW *UI_term)
{
	unsigned int end_index=0, cur_pos=0, key, argc, last_curx, history_buffs_index=0, retval;
	char *argv[max_amount_of_user_arg] = {NULL};
	GQueue *hist_buffs = g_queue_new();
	g_queue_push_head(hist_buffs, g_slice_alloc0(sizeof(history_buffer_entry)));
	gpointer nth_node = NULL;
	char *usr_in_buff = ((history_buffer_entry *)g_queue_peek_head(hist_buffs))->usr_in_buff;
	char temp_usr_in_buff[user_inp_buf_size];

	noecho();//disable echo
	raw();//getch without return
	keypad(stdscr, TRUE);
	scrollok(stdscr, TRUE);
	wprintw(UI_term, "press '?' for help.\n");
	wprintw(UI_term, "][ ");
	while(running)
	{
		key = getch();// get the user's entrance
		switch(key)
		{
			case 3 ://ctrl + c clear buffer
				move(getcury(stdscr),3);
				clrtoeol();
				end_index = 0;
				cur_pos = 0;
				for(int i=0;i<user_inp_buf_size;i++)
					usr_in_buff[i] = '\0';
				break;
			case 9 ://ctrl + i print CAN-if
				wprintw(UI_term, "\nSDAQ_psim interfacing with \"%s\"",CAN_if);
				wprintw(UI_term, "\n][ %s",usr_in_buff);
				break;
			case 17 ://ctrl + q
				running = 0;
				break;
			case 12 : //ctrl + l
				clear();
				wprintw(UI_term, "][ %s",usr_in_buff);
				cur_pos = end_index;
				break;
			case '?' : //user request for help
				last_curx = getcurx(stdscr);
				retval = shell_help();
				refresh();
				clear();
				if(retval)
					wprintw(UI_term, "Terminal size too small to print help!!!\n");
				wprintw(UI_term, "][ %s",usr_in_buff);
				move(retval,last_curx);
				break;
			case KEY_UP:
				if((nth_node = g_queue_peek_nth(hist_buffs,history_buffs_index+1)))
				{
					if(!history_buffs_index)
						memcpy(temp_usr_in_buff, usr_in_buff, sizeof(char)*user_inp_buf_size);
					memset(usr_in_buff, '\0', sizeof(char)*user_inp_buf_size);
					strcpy(usr_in_buff, ((history_buffer_entry *)nth_node)->usr_in_buff);
					history_buffs_index++;
					move(getcury(stdscr),3);
					clrtoeol();
					wprintw(UI_term, "%s",usr_in_buff);
					end_index = strlen(usr_in_buff);
					cur_pos = end_index;
				}
				break;
			case KEY_DOWN:
				if((nth_node = g_queue_peek_nth(hist_buffs,history_buffs_index-1)))
				{
					memset(usr_in_buff, '\0', sizeof(char)*user_inp_buf_size);
					strcpy(usr_in_buff, ((history_buffer_entry *)nth_node)->usr_in_buff);
					history_buffs_index--;
					move(getcury(stdscr),3);
					clrtoeol();
					if(!history_buffs_index)
					{
						memcpy(usr_in_buff, temp_usr_in_buff, sizeof(char)*user_inp_buf_size);
						move(getcury(stdscr),3);
					}
					wprintw(UI_term, "%s",usr_in_buff);
					end_index = strlen(usr_in_buff);
					cur_pos = end_index;
				}

				break;
			case KEY_LEFT:
				if(cur_pos)
				{
					move(getcury(stdscr),getcurx(stdscr)-1);
					cur_pos--;
				}
				break;
			case KEY_RIGHT:
				if(cur_pos<end_index)
				{
					move(getcury(stdscr),getcurx(stdscr)+1);
					cur_pos++;
				}
				break;
			case KEY_BACKSPACE :
				if(cur_pos)
				{
					for(unsigned int i=cur_pos-1;i<=end_index;i++)
						usr_in_buff[i] = usr_in_buff[i+1];
					move(getcury(stdscr),getcurx(stdscr)-1);//move cursor one left
					clrtoeol(); //clear from buffer to the end of line
					end_index--;
					cur_pos--;
					usr_in_buff[end_index] = '\0';
					wprintw(UI_term, "%s", usr_in_buff + cur_pos);
					move(getcury(stdscr),getcurx(stdscr)-(end_index-cur_pos));
				}
				break;
			case KEY_DC ://Delete key
				if(cur_pos<end_index)
				{
					for(unsigned int i=cur_pos;i<=end_index;i++)
						usr_in_buff[i] = usr_in_buff[i+1];
					end_index--;
					clrtoeol();
					wprintw(UI_term, "%s", usr_in_buff + cur_pos);
					move(getcury(stdscr),getcurx(stdscr)-(end_index-cur_pos));
					usr_in_buff[end_index] = '\0';
				}
				break;
			case KEY_HOME ://Home key
				cur_pos = 0;
				move(getcury(stdscr),3);
				break;
			case KEY_END ://End key
				cur_pos = end_index;
				move(getcury(stdscr),3+end_index);
				break;
			case '\r' :
			case '\n' ://return or enter : Command decode and execution
				usr_in_buff[end_index] = '\0';
				move(getcury(stdscr),getcurx(stdscr)+(end_index-cur_pos));
				argc = user_inp_dec(argv, usr_in_buff);
				//user_com(argc, argv, start_sn, num_of_pSDAQ, pSDAQs_mem);
				wprintw(UI_term, "\n][ ");
				end_index = 0;
				cur_pos = 0;
				if(*usr_in_buff)//make new entry in the history queue only if the current usr_in_buff is not empty
				{
					g_queue_push_head(hist_buffs, g_slice_alloc0(sizeof(history_buffer_entry)));
					usr_in_buff = ((history_buffer_entry *)g_queue_peek_head(hist_buffs))->usr_in_buff;
					if(g_queue_get_length(hist_buffs)>history_buffs_length)
						history_buff_free_node(g_queue_pop_tail(hist_buffs));
				}
				history_buffs_index = 0;
				break;
			default : //normal key press
				if(isprint(key))
				{
					if(end_index<user_inp_buf_size-1)
					{	//check if cursor has moved from the user
						if(cur_pos<end_index)
						{	//roll right side of the buffer by one position
							for(int i=end_index; i>=cur_pos && i>=0; i--)
								usr_in_buff[i+1] = usr_in_buff[i];
						}
						usr_in_buff[cur_pos] = key; // add new pressed key to the buffer
						end_index++;
						wprintw(UI_term, "%s", usr_in_buff+cur_pos);
						cur_pos++;
						move(getcury(stdscr),getcurx(stdscr)-(end_index-cur_pos));
					}
				}
				else
				{
					wprintw(UI_term, "Special key = %d",key);
					wprintw(UI_term, "\n][ ");
				}
				break;
		}
	}
	//g_queue_foreach(hist_buffs, print_hist_buffs, NULL);
	g_queue_free_full(hist_buffs,history_buff_free_node);//free the allocated space of the history buffers
	return;
}