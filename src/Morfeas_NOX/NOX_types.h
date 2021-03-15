/*
File: UNI_NOX_types.h, Contain the Declaration for data types that related to NOX sensors, Part of Morfeas_project.
Copyright (C) 12021-12022  Sam harry Tzavaras

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

typedef struct{
	short NOx_value;
	short O2_value;
	unsigned Supply_flag :2;
	unsigned Heater_flag :2;
	unsigned NOx_flag :2;
	unsigned Oxygen_flag :2;
	unsigned Heater_error :5;
	unsigned Heater_mode :2;
	unsigned NU_1 :1;
	unsigned NOx_error :5;
	unsigned NU_2 :3;
	unsigned O2_error :5;
	unsigned NU_3 :3;
} NOx_frame;