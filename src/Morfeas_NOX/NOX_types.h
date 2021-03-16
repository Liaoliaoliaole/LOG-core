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

#define NOX_Sensor_high_addr 0x18F00F52
#define NOX_Sensor_low_addr 0x18F00E51

extern const char *Supply_valid_str[];
extern const char *Heater_valid_str[];
extern const char *NOx_valid_str[];
extern const char *Oxygen_valid_str[];
extern const char *Heater_mode_str[];

#pragma pack(push, 1)//use pragma pack() to pack the following structs to 1 byte size (aka no zero padding)
typedef struct{
	unsigned short NOx_value;
	unsigned short O2_value;
	unsigned Supply_valid :2;
	unsigned Heater_valid :2;
	unsigned NOx_valid :2;
	unsigned Oxygen_valid :2;
	unsigned Heater_error :5;
	unsigned Heater_mode :2;
	unsigned NU_1 :1;
	unsigned NOx_error :5;
	unsigned NU_2 :3;
	unsigned O2_error :5;
	unsigned NU_3 :3;
} NOx_RX_frame;

typedef struct{
	unsigned char res[7];
	unsigned char start_code;
} NOx_TX_frame;
#pragma pack(pop)//Disable packing