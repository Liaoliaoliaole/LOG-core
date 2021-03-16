/*
File: NOX_func.c, Implementation of functions for NOX (CANBus), Part of Morfeas_project.
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


const char *Supply_valid_str[]={
	"Supply not in range",
	"Supply in range",
	"Not used => Error",
	"Not available (=Initial value)"
};
const char *Heater_valid_str[]={
	"Sensor not at temperature",
	"Sensor at operating temperature",
	"Not used => Error",
	"Not available (=Initial value)"
};
const char *NOx_valid_str[]={
	"NOx-signal not valid",
	"NOx-signal valid",
	"Not used => Error",
	"Not available (=Initial value)"
};
const char *Oxygen_valid_str[]={
	"O2-signal not valid",
	"O2-signal valid",
	"Not used => Error",
	"Not available (=Initial value)"
};
const char *Heater_mode_str[]={
	"Automatic mode",
	"Heating up slope 3 or 4",
	"Heating up slope 1 or 2",
	"Heater off / Preheating mode"
};