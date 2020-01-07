/*
File: Morfeas_run_check.c Implementation of check_run function.
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

int check_already_run(char *prog_name)
{
	char out_str[128], cmd[128], *tok, i=1;

	sprintf(cmd, "pidof %s",prog_name);
	FILE *out = popen(cmd, "r");
	fgets(out_str, sizeof(out_str), out);
	pclose(out);
	tok = strtok(out_str, " ");
	while((tok = strtok(NULL, " ")))
		i++;
	return i>1? 1 : 0;
}

int check_already_run_onBus(char *prog_name, char *bus_name)
{
	char out_str[1024] = {0}, cmd[128], *tok, i=1;

	sprintf(cmd, "ps aux | grep --color=none \"%s %s\"",prog_name, bus_name);
	FILE *out = popen(cmd, "r");
	fread(out_str, sizeof(out_str), sizeof(char), out);
	pclose(out);
	tok = strtok(out_str, "\n");
	while((tok = strtok(NULL, "\n")))
		i++;
	return i>3? 1 : 0;
}
