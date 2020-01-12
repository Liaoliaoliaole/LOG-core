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

#define _GNU_SOURCE
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
pthread_mutex_t start = PTHREAD_MUTEX_INITIALIZER;

//print the Usage manual
void print_usage(char *prog_name);


static void stopHandler(int sign)
{
	running = 0;
}

int main(int argc, char *argv[])
{
	char *config_path = NULL, *loggers_dir_path, path_buff[128]={0};
	DIR *loggers_dir;
	xmlDoc *doc;//XML DOC tree pointer
	xmlNode *xml_node, *root_element; //XML root Node

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
			if((loggers_dir_path = XML_node_get_content(root_element, "LOGGERS_DIR")))
			{
				//make Morfeas_Loggers_Directory
				sprintf(path_buff, "%s/Morfeas_Loggers/", loggers_dir_path);
				mkdir(path_buff, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
				if((loggers_dir = opendir(path_buff)))
				{
					closedir(loggers_dir);
					//Get components from Config XML
					if((xml_node = get_XML_node(root_element, "COMPONENTS")))
					{
						while(xml_node)
						{
							xml_node = xml_node->next;
						}
					}
					else
					{
						printf("\"COMPONENTS\" XML node not found\n");
						xmlFreeDoc(doc);//Free XML Doc
						exit(EXIT_FAILURE);
					}
				}
				else
				{
					perror("Error on Loggers directory: ");
					xmlFreeDoc(doc);//Free XML Doc
					exit(EXIT_FAILURE);
				}
			}
			else
			{
				printf("\"LOGGERS_DIR\" XML node not found\n");
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
		xmlFreeDoc(doc);//Free XML Doc
		exit(EXIT_FAILURE);
	}

	while(running)
		sleep(1);

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
