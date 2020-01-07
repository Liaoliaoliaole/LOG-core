/*
File: Morfeas_run_check.h Declaration of check_run function.
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

//-- Function that detects if other program with the same name already running --//
//-- return 0 if no other program found --// 
int check_already_run(char *prog_name);
//-- Function that detects if other program with the same name already running on the same bus --//
//-- return 0 if no other program found --// 
int check_already_run_onBus(char *prog_name, char *bus_name);