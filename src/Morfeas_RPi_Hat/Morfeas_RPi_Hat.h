/*
File: Morfeas_RPi_Hat.h  Declaration of functions related to Morfeas_RPi_Hat.
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
#define YELLOW_LED 19
#define RED_LED 13
	//---- LEDs related ----//
//Init Morfeas_RPi_Hat LEDs, return 1 if sysfs files exist, 0 otherwise.
int led_init();
//Write value to Morfeas_RPi_Hat LED, return 0 if write was success, -1 otherwise.
int GPIOWrite(int LED_name, int value);
//Read value of Morfeas_RPi_Hat LED by name, return value of the LED, or -1 if read failed.
int GPIORead(int LED_name);
	//---- I2C device related ----//

