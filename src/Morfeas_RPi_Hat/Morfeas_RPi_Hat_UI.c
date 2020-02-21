/*
Program: Morfeas_RPi_Hat_if.c  Implementation of supporting software for Morfeas_RPi_Hat.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <arpa/inet.h>

#include "../Supplementary/Morfeas_run_check.h"
#include "Morfeas_RPi_Hat.h"

//Global variables

int main(int argc, char *argv[])
{
	struct Morfeas_RPi_Hat_EEPROM_SDAQnet_Port_config Ports_config[4] = {0};
	struct Morfeas_RPi_Hat_Port_meas Ports_meas[4] = {0};
	
	//Check if program already runs.
	if(check_already_run(argv[0]))
	{
		fprintf(stderr, "%s Already Running!!!\n", argv[0]);
		exit(EXIT_SUCCESS);
	}
	//Check if Morfeas_SDAQ_if running.
	if(check_already_run("Morfeas_SDAQ_if"))
	{
		fprintf(stderr, "Morfeas_SDAQ_if detected to run, %s can't run!!!\n", argv[0]);
		exit(EXIT_SUCCESS);
	}
/*
//Struct for EEPROM(24AA08) data
struct Morfeas_RPi_Hat_EEPROM_SDAQnet_Port_config{
	struct last_port_calibration_date{
		unsigned char year;
		unsigned char month;
		unsigned char day;
	}last_cal_date;
	char volt_meas_offset;
	char curr_meas_offset;
	float curr_meas_scaler;
	unsigned char checksum;
};
*/	
	
	Ports_config[0].last_cal_date.year = 1;
	Ports_config[0].last_cal_date.month = 3;
	Ports_config[0].last_cal_date.day = 5;
	Ports_config[0].volt_meas_offset = 0;
	Ports_config[0].curr_meas_offset = 0;
	Ports_config[0].curr_meas_scaler = 0.001222;
	if(write_port_config(&Ports_config[0], 0, I2C_BUS_NUM))
		printf("%s",Morfeas_hat_error());
	
	if(write_port_config(&Ports_config[0], 1, I2C_BUS_NUM))
		printf("%s",Morfeas_hat_error());
	/*
	if(erase_EEPROM(0, I2C_BUS_NUM))
		printf("%s",Morfeas_hat_error());
	*/
	//Get EEPROM config data
	for(int i=0;i<4;i++)
	{
		if(read_port_config(&Ports_config[i], i, I2C_BUS_NUM))
			printf("%s",Morfeas_hat_error());
	}
	MAX9611_init(1,1);
	while(1)
	{
		get_port_meas(&Ports_meas[0], 0, I2C_BUS_NUM);
		printf("\nPort0_voltage= %.1fV\n",Ports_meas[0].port_voltage*MAX9611_volt_meas_scaler);
		printf("Port0_current= %.3fA\n",(Ports_meas[0].port_current)*0.001222);
		printf("Die0_temperature= %.0f°C\n",Ports_meas[0].temperature*MAX9611_temp_scaler);
		
		get_port_meas(&Ports_meas[1], 1, I2C_BUS_NUM);
		printf("\nPort1_voltage= %.1fV\n",Ports_meas[1].port_voltage*MAX9611_volt_meas_scaler);
		printf("Port1_current= %.3fA\n",(Ports_meas[1].port_current)*0.001222);
		printf("Die1_temperature= %.0f°C\n",Ports_meas[1].temperature*MAX9611_temp_scaler);
		
		sleep(1);
	}
	return 0;
}
