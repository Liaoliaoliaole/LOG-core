/*
File "Morfeas_JSON.c" part of Morfeas project, contains implementation of "Morfeas_JSON.h".
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
#include <string.h>
#include <unistd.h>
#include <time.h>

//Header for cJSON
#include <cjson/cJSON.h>
//Include Functions implementation header
#include "Types.h"

int logstat_json(char *logstat_path, void *stats_arg)
{
	if(!logstat_path)
		return 1;
	struct Morfeas_SDAQ_if_stats *stats = (struct Morfeas_SDAQ_if_stats *) stats_arg;
	FILE * pFile;
	char *logstat_path_and_name;
	logstat_path_and_name = (char *) malloc(sizeof(char) * strlen(logstat_path) + strlen(stats->CAN_IF_name));
	sprintf(logstat_path_and_name,"%s%s",logstat_path,stats->CAN_IF_name);
	//cJSON related variables
	char *JSON_str = NULL;
	cJSON *root = NULL;
    cJSON *fmt = NULL;
    printf("Version: %s\n", cJSON_Version());
	
	root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "name", cJSON_CreateString("Jack (\"Bee\") Nimble"));
    cJSON_AddItemToObject(root, "format", fmt = cJSON_CreateObject());
    cJSON_AddStringToObject(fmt, "type", "rect");
    cJSON_AddNumberToObject(fmt, "width", 1920);
    cJSON_AddNumberToObject(fmt, "height", 1080);
    cJSON_AddFalseToObject (fmt, "interlace");
    cJSON_AddNumberToObject(fmt, "frame rate", 24);
	
	JSON_str = cJSON_Print(root);
	//JSON_str = cJSON_PrintUnformatted(root);
	pFile = fopen (logstat_path_and_name, "w");
	if(pFile)
	{
		fputs(JSON_str, pFile);
		fclose (pFile);
	}
	cJSON_Delete(root);
	free(JSON_str);
	free(logstat_path_and_name);
	return 0;
}