/*
File: Morfeas_daemon.c, Implementation of Morfeas_daemon program, Part of Morfeas_project.
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
#define VERSION "0.1" /*Release Version of Morfeas_daemon*/
#define max_num_of_threads 18

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "Morfeas_Types.h"
#include "Supplementary/Morfeas_run_check.h"
#include "Supplementary/Morfeas_XML.h"

//Global variables
static unsigned char running = 1;
pthread_mutex_t thread_make_lock = PTHREAD_MUTEX_INITIALIZER;

//print the Usage manual
void print_usage(char *prog_name);
//Thread function 
void * Morfeas_thread(void *varg_pt);

static void stopHandler(int sign)
{
	running = 0;
}

int main(int argc, char *argv[])
{
	unsigned char nodes_cnt=0;
	char *config_path = NULL, *path_buff;
	DIR *loggers_dir;
	xmlDoc *doc;//XML DOC tree pointer
	xmlNode *Morfeas_component, *root_element; //XML root Node
	xmlChar* node_attr; //Value of Node's Attribute 
	//variables for threads
	pthread_t Threads_ids[max_num_of_threads] = {0}, *Threads_ids_ind = Threads_ids;

	int c;
	//Get options
	while ((c = getopt (argc, argv, "hVc:")) != -1)
	{
		switch (c)
		{
			case 'h'://help
				print_usage(argv[0]);
				exit(EXIT_SUCCESS);
			case 'V'://Version
				printf(VERSION"\n");
				exit(EXIT_SUCCESS);
			case 'c'://configuration XML file
				config_path = optarg;
				break;
			case '?':
				print_usage(argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	//Check if program already runs in other instance.
	if(check_already_run(argv[0]))
	{
		printf("%s Already running !!!\n", argv[0]);
		exit(EXIT_SUCCESS);
	}

	if(!config_path || access(config_path, R_OK | F_OK ) || !strstr(config_path, ".xml"))
	{
		printf("Configuration File not defined or invalid!!!\n");
		exit(EXIT_FAILURE);
	}

	//Install stopHandler as the signal handler for SIGINT and SIGTERM signals.
	signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

	//Configuration XML parsing and validation check
	if(!Morfeas_XML_parsing(config_path, &doc))
	{
		root_element = xmlDocGetRootElement(doc);
		if(!Morfeas_daemon_config_valid(root_element))
		{
			//make Morfeas_Loggers_Directory
			path_buff = XML_node_get_content(root_element, "LOGGERS_DIR");
			mkdir(path_buff, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
			if((loggers_dir = opendir(path_buff)))
			{
				closedir(loggers_dir);
				//Get Morfeas component from Configuration XML
				Morfeas_component = (get_XML_node(root_element, "COMPONENTS"))->children;
				while(Morfeas_component && nodes_cnt<max_num_of_threads)
				{
					if (Morfeas_component->type == XML_ELEMENT_NODE)
					{
						if((node_attr = xmlGetProp(Morfeas_component, BAD_CAST"Disable")))
						{
							if(!strcmp((char *)node_attr, "false"))
							{
								printf("Node name: %s\n", Morfeas_component->name);
								pthread_mutex_lock(&thread_make_lock);
								pthread_create(Threads_ids_ind, NULL, Morfeas_thread, Morfeas_component);
								Threads_ids_ind++;
							}	
							xmlFree(node_attr);
							nodes_cnt++;
						}
					}
					Morfeas_component = Morfeas_component->next;
				}
				if(nodes_cnt>=max_num_of_threads)
					printf("Max_amount of thread reached!!!\n");
			}
			else
			{
				perror("Error on creation of Loggers directory: ");
				xmlFreeDoc(doc);//Free XML Doc
				exit(EXIT_FAILURE);
			}
		}
		else
		{
			printf("Data Validation of The configuration XML file failed!!!\n");
			xmlFreeDoc(doc);//Free XML Doc
			exit(EXIT_FAILURE);
		}
		xmlFreeDoc(doc);//Free XML Doc
	}
	else
	{
		printf("XML Parsing of The configuration XML file failed!!!\n");
		exit(EXIT_FAILURE);
	}
	//sleep_loop
	while(running)
		sleep(1);
	
	//Wait until all threads ends
	for(int i=0; i<max_num_of_threads; i++)
	{
		pthread_join(Threads_ids[i], NULL);// wait for thread to finish
		pthread_detach(Threads_ids[i]);//deallocate thread memory 
	}
	xmlCleanupParser();
	xmlMemoryDump();
	return 0;
}

//print the Usage manual
void print_usage(char *prog_name)
{
	const char preamp[] = {
	"\tProgram: Morfeas_daemon  Copyright (C) 12019-12020  Sam Harry Tzavaras\n"
    "\tThis program comes with ABSOLUTELY NO WARRANTY; for details see LICENSE.\n"
    "\tThis is free software, and you are welcome to redistribute it\n"
    "\tunder certain conditions; for details see LICENSE.\n"
	};
	const char manual[] = {
		"Options:\n"
		"           -h : Print help.\n"
		"           -V : Version.\n"
		"           -c : Path to configuration XML file.\n"
	};
	printf("%s\nUsage: %s [Options]\n\n%s",preamp, prog_name, manual);
	return;
}

//Thread Function, Decode varg_pt, and start the Morfeas Component program.
void * Morfeas_thread(void *varg_pt)
{
	//Unlock threading making
	pthread_mutex_unlock(&thread_make_lock);
	return NULL;
}
