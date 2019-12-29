/*
File: Morfeas_Logger.h Implementation of Morfeas_Logger Functions.
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
#include <stdarg.h>
#include <time.h>

void Logger(const char *fmt, ...)
{
	time_t now = time(NULL);
	struct tm * timeinfo;
	char buffer[100];
	timeinfo = localtime(&now);
	strftime(buffer,sizeof(buffer),"%F %a %T",timeinfo);
	printf("(%s): ", buffer);
	va_list arg;
    va_start(arg, fmt);
    	vfprintf(stdout, fmt, arg);
    va_end(arg);
}
