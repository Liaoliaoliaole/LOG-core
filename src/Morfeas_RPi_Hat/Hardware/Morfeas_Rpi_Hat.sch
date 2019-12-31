EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "Morfeas RPi Hat"
Date "2019-12-27"
Rev "V1.0"
Comp ""
Comment1 ""
Comment2 "https://www.tapr.org/ohl.html"
Comment3 "Licence: TAPR Open Hardware License"
Comment4 "Author: Sam Harry Tzavaras"
$EndDescr
$Comp
L Connector_Generic:Conn_02x20_Odd_Even J2
U 1 1 5E041881
P 6250 3850
F 0 "J2" H 6300 4967 50  0000 C CNN
F 1 "RPi_header" H 6300 4876 50  0000 C CNN
F 2 "Connector_PinSocket_2.54mm:PinSocket_2x20_P2.54mm_Vertical" H 6250 3850 50  0001 C CNN
F 3 "~" H 6250 3850 50  0001 C CNN
	1    6250 3850
	1    0    0    -1  
$EndComp
$Comp
L power:+5V #PWR014
U 1 1 5E047DD2
P 6650 2850
F 0 "#PWR014" H 6650 2700 50  0001 C CNN
F 1 "+5V" H 6665 3023 50  0000 C CNN
F 2 "" H 6650 2850 50  0001 C CNN
F 3 "" H 6650 2850 50  0001 C CNN
	1    6650 2850
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR08
U 1 1 5E048E34
P 6550 3150
F 0 "#PWR08" H 6550 2900 50  0001 C CNN
F 1 "GND" V 6550 2950 50  0000 C CNN
F 2 "" H 6550 3150 50  0001 C CNN
F 3 "" H 6550 3150 50  0001 C CNN
	1    6550 3150
	0    -1   -1   0   
$EndComp
$Comp
L Morfeas_Rpi_Hat:DS3231SN#T&R U1
U 1 1 5E055AC1
P 9500 3950
F 0 "U1" H 9150 4400 50  0000 C CNN
F 1 "DS3231" H 9250 3500 50  0000 C CNN
F 2 "Project's_libs:SOIC127P1032X265-16N" H 9100 3350 50  0001 L BNN
F 3 "Maxim Intergrated" H 9100 3250 50  0001 L BNN
	1    9500 3950
	1    0    0    -1  
$EndComp
$Comp
L Morfeas_Rpi_Hat:24AA08T-I_OT U5
U 1 1 5E05839E
P 10300 5550
F 0 "U5" H 9950 5900 60  0000 L CNN
F 1 "24AA08" H 10350 5200 60  0000 L CNN
F 2 "Morfeas_RPi_Hat:24AA08T-I&slash_OT" H 10650 5050 60  0001 C CNN
F 3 "" H 9650 6300 60  0000 C CNN
	1    10300 5550
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR018
U 1 1 5E058B24
P 10200 4350
F 0 "#PWR018" H 10200 4100 50  0001 C CNN
F 1 "GND" H 10205 4177 50  0000 C CNN
F 2 "" H 10200 4350 50  0001 C CNN
F 3 "" H 10200 4350 50  0001 C CNN
	1    10200 4350
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR027
U 1 1 5E059341
P 10300 5950
F 0 "#PWR027" H 10300 5700 50  0001 C CNN
F 1 "GND" H 10305 5777 50  0000 C CNN
F 2 "" H 10300 5950 50  0001 C CNN
F 3 "" H 10300 5950 50  0001 C CNN
	1    10300 5950
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR021
U 1 1 5E05AAF9
P 5200 6800
F 0 "#PWR021" H 5200 6650 50  0001 C CNN
F 1 "+3.3V" H 5215 6973 50  0000 C CNN
F 2 "" H 5200 6800 50  0001 C CNN
F 3 "" H 5200 6800 50  0001 C CNN
	1    5200 6800
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR020
U 1 1 5E05B115
P 4600 7300
F 0 "#PWR020" H 4600 7050 50  0001 C CNN
F 1 "GND" H 4605 7127 50  0000 C CNN
F 2 "" H 4600 7300 50  0001 C CNN
F 3 "" H 4600 7300 50  0001 C CNN
	1    4600 7300
	1    0    0    -1  
$EndComp
Wire Wire Line
	10100 4250 10200 4250
Wire Wire Line
	10200 4250 10200 4350
Wire Wire Line
	10100 3650 10200 3650
Wire Wire Line
	10200 3650 10200 3550
$Comp
L Device:Battery_Cell BT1
U 1 1 5E05C7F6
P 10550 4050
F 0 "BT1" H 10668 4146 50  0000 L CNN
F 1 "Battery_Cell" H 10668 4055 50  0000 L CNN
F 2 "Morfeas_RPi_hat:CH252032LF" V 10550 4110 50  0001 C CNN
F 3 "~" V 10550 4110 50  0001 C CNN
	1    10550 4050
	1    0    0    -1  
$EndComp
Wire Wire Line
	10100 3750 10550 3750
Wire Wire Line
	10550 3750 10550 3850
$Comp
L power:GND #PWR019
U 1 1 5E05FB69
P 10550 4200
F 0 "#PWR019" H 10550 3950 50  0001 C CNN
F 1 "GND" H 10555 4027 50  0000 C CNN
F 2 "" H 10550 4200 50  0001 C CNN
F 3 "" H 10550 4200 50  0001 C CNN
	1    10550 4200
	1    0    0    -1  
$EndComp
Wire Wire Line
	10550 4150 10550 4200
Text GLabel 8800 4050 0    50   Input ~ 0
SDA
Text GLabel 8800 4150 0    50   Input ~ 0
SCL
Wire Wire Line
	8800 4050 8900 4050
Wire Wire Line
	8800 4150 8900 4150
Wire Wire Line
	5100 6900 5200 6900
Wire Wire Line
	5200 6900 5200 6800
$Comp
L power:+5V #PWR015
U 1 1 5E062946
P 4000 6800
F 0 "#PWR015" H 4000 6650 50  0001 C CNN
F 1 "+5V" H 4015 6973 50  0000 C CNN
F 2 "" H 4000 6800 50  0001 C CNN
F 3 "" H 4000 6800 50  0001 C CNN
	1    4000 6800
	1    0    0    -1  
$EndComp
Wire Wire Line
	4100 6900 4000 6900
Wire Wire Line
	4000 6900 4000 6800
$Comp
L Morfeas_Rpi_Hat:MAX9611AUB+ U6
U 1 1 5E065DED
P 1850 1800
F 0 "U6" H 1550 2250 60  0000 C CNN
F 1 "MAX9611" H 1600 1300 60  0000 C CNN
F 2 "Morfeas_RPi_Hat:MAX9611AUB&plus_" H 2150 1150 60  0001 C CNN
F 3 "" H 750 2000 60  0000 C CNN
	1    1850 1800
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR032
U 1 1 5E0672FC
P 1850 2550
F 0 "#PWR032" H 1850 2300 50  0001 C CNN
F 1 "GND" H 1855 2377 50  0000 C CNN
F 2 "" H 1850 2550 50  0001 C CNN
F 3 "" H 1850 2550 50  0001 C CNN
	1    1850 2550
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR034
U 1 1 5E067682
P 2550 1400
F 0 "#PWR034" H 2550 1250 50  0001 C CNN
F 1 "+3.3V" H 2565 1573 50  0000 C CNN
F 2 "" H 2550 1400 50  0001 C CNN
F 3 "" H 2550 1400 50  0001 C CNN
	1    2550 1400
	1    0    0    -1  
$EndComp
Wire Wire Line
	2500 1650 2550 1650
$Comp
L Device:C_Small C13
U 1 1 5E068B8A
P 2700 1600
F 0 "C13" H 2700 1700 50  0000 L CNN
F 1 "100nf" H 2450 1500 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 2700 1600 50  0001 C CNN
F 3 "~" H 2700 1600 50  0001 C CNN
	1    2700 1600
	1    0    0    -1  
$EndComp
Connection ~ 2550 1450
Wire Wire Line
	2550 1450 2700 1450
Wire Wire Line
	2700 1450 2700 1500
Wire Wire Line
	2550 1450 2550 1400
Wire Wire Line
	2550 1450 2550 1650
$Comp
L power:GND #PWR036
U 1 1 5E06CD8E
P 2700 1700
F 0 "#PWR036" H 2700 1450 50  0001 C CNN
F 1 "GND" H 2700 1550 50  0000 C CNN
F 2 "" H 2700 1700 50  0001 C CNN
F 3 "" H 2700 1700 50  0001 C CNN
	1    2700 1700
	1    0    0    -1  
$EndComp
$Comp
L Device:R_US R1
U 1 1 5E06E0BD
P 1850 1100
F 0 "R1" V 1645 1100 50  0000 C CNN
F 1 "22mΩ" V 1736 1100 50  0000 C CNN
F 2 "Resistor_SMD:R_2512_6332Metric" V 1890 1090 50  0001 C CNN
F 3 "~" H 1850 1100 50  0001 C CNN
	1    1850 1100
	0    1    1    0   
$EndComp
Wire Wire Line
	2050 1100 2000 1100
Wire Wire Line
	1650 1100 1700 1100
Wire Wire Line
	10750 5550 10800 5550
Wire Wire Line
	10800 5550 10800 5600
Text GLabel 1200 2000 0    50   Input ~ 0
SDA
Text GLabel 9750 5600 0    50   Input ~ 0
SCL
Wire Wire Line
	9750 5600 9850 5600
Wire Wire Line
	9750 5450 9850 5450
Connection ~ 2050 1100
Wire Wire Line
	3650 1100 3650 1000
Wire Wire Line
	3250 1100 3650 1100
$Comp
L power:+24V #PWR042
U 1 1 5E081606
P 3650 1000
F 0 "#PWR042" H 3650 850 50  0001 C CNN
F 1 "+24V" H 3665 1173 50  0000 C CNN
F 2 "" H 3650 1000 50  0001 C CNN
F 3 "" H 3650 1000 50  0001 C CNN
	1    3650 1000
	1    0    0    -1  
$EndComp
Wire Wire Line
	2850 1100 2050 1100
Wire Wire Line
	1650 1100 1300 1100
Connection ~ 1650 1100
Text GLabel 1200 2100 0    50   Input ~ 0
SCL
Text GLabel 9750 5450 0    50   Input ~ 0
SDA
$Comp
L Device:D_ALT D1
U 1 1 5E08FF22
P 2300 6900
F 0 "D1" H 2346 6821 50  0000 R CNN
F 1 "1N4007" H 2450 7000 50  0000 R CNN
F 2 "Morfeas_RPi_hat:DO-214AC(SMA)" H 2300 6900 50  0001 C CNN
F 3 "~" H 2300 6900 50  0001 C CNN
	1    2300 6900
	-1   0    0    1   
$EndComp
$Comp
L power:+3.3V #PWR026
U 1 1 5E059A1A
P 10300 5050
F 0 "#PWR026" H 10300 4900 50  0001 C CNN
F 1 "+3.3V" H 10315 5223 50  0000 C CNN
F 2 "" H 10300 5050 50  0001 C CNN
F 3 "" H 10300 5050 50  0001 C CNN
	1    10300 5050
	1    0    0    -1  
$EndComp
Wire Wire Line
	10300 5050 10300 5100
Wire Wire Line
	10400 5100 10300 5100
Connection ~ 10300 5100
Wire Wire Line
	10300 5100 10300 5150
$Comp
L Device:C_Small C8
U 1 1 5E09628B
P 10500 5100
F 0 "C8" V 10400 5100 50  0000 C CNN
F 1 "100nf" V 10600 5100 50  0000 C CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 10500 5100 50  0001 C CNN
F 3 "~" H 10500 5100 50  0001 C CNN
	1    10500 5100
	0    1    1    0   
$EndComp
$Comp
L power:GND #PWR031
U 1 1 5E0769C0
P 10800 5600
F 0 "#PWR031" H 10800 5350 50  0001 C CNN
F 1 "GND" H 10805 5427 50  0000 C CNN
F 2 "" H 10800 5600 50  0001 C CNN
F 3 "" H 10800 5600 50  0001 C CNN
	1    10800 5600
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR028
U 1 1 5E097593
P 10600 5100
F 0 "#PWR028" H 10600 4850 50  0001 C CNN
F 1 "GND" H 10605 4927 50  0000 C CNN
F 2 "" H 10600 5100 50  0001 C CNN
F 3 "" H 10600 5100 50  0001 C CNN
	1    10600 5100
	0    -1   -1   0   
$EndComp
$Comp
L Device:R_US R9
U 1 1 5E0981EB
P 3400 1400
F 0 "R9" V 3500 1400 50  0000 C CNN
F 1 "2.2kΩ" V 3300 1400 50  0000 C CNN
F 2 "Resistor_SMD:R_1206_3216Metric" V 3440 1390 50  0001 C CNN
F 3 "~" H 3400 1400 50  0001 C CNN
	1    3400 1400
	0    -1   -1   0   
$EndComp
$Comp
L Device:R_US R7
U 1 1 5E099BBF
P 3050 1650
F 0 "R7" H 3150 1600 50  0000 C CNN
F 1 "1.2kΩ" H 3200 1700 50  0000 C CNN
F 2 "Resistor_SMD:R_1206_3216Metric" V 3090 1640 50  0001 C CNN
F 3 "~" H 3050 1650 50  0001 C CNN
	1    3050 1650
	-1   0    0    1   
$EndComp
Wire Wire Line
	3050 1500 3050 1400
Wire Wire Line
	3250 1400 3150 1400
Connection ~ 3650 1100
Wire Wire Line
	3550 1400 3650 1400
Wire Wire Line
	3650 1400 3650 1100
Wire Wire Line
	2500 1900 3050 1900
Wire Wire Line
	3050 1900 3050 1800
$Comp
L Device:R_US R5
U 1 1 5E09FA87
P 3000 2100
F 0 "R5" V 3100 2100 50  0000 C CNN
F 1 "18kΩ" V 2900 2100 50  0000 C CNN
F 2 "Resistor_SMD:R_1206_3216Metric" V 3040 2090 50  0001 C CNN
F 3 "~" H 3000 2100 50  0001 C CNN
	1    3000 2100
	0    -1   -1   0   
$EndComp
$Comp
L Device:R_US R3
U 1 1 5E0A0977
P 2750 2350
F 0 "R3" H 2850 2300 50  0000 C CNN
F 1 "1.3kΩ" H 2900 2400 50  0000 C CNN
F 2 "Resistor_SMD:R_1206_3216Metric" V 2790 2340 50  0001 C CNN
F 3 "~" H 2750 2350 50  0001 C CNN
	1    2750 2350
	1    0    0    1   
$EndComp
$Comp
L power:GND #PWR038
U 1 1 5E0A1353
P 2750 2600
F 0 "#PWR038" H 2750 2350 50  0001 C CNN
F 1 "GND" H 2755 2427 50  0000 C CNN
F 2 "" H 2750 2600 50  0001 C CNN
F 3 "" H 2750 2600 50  0001 C CNN
	1    2750 2600
	1    0    0    -1  
$EndComp
Wire Wire Line
	2750 2200 2750 2100
Wire Wire Line
	2850 2100 2750 2100
Connection ~ 2750 2100
$Comp
L power:+3.3V #PWR040
U 1 1 5E0A2C24
P 3250 2100
F 0 "#PWR040" H 3250 1950 50  0001 C CNN
F 1 "+3.3V" V 3265 2228 50  0000 L CNN
F 2 "" H 3250 2100 50  0001 C CNN
F 3 "" H 3250 2100 50  0001 C CNN
	1    3250 2100
	0    1    1    0   
$EndComp
Wire Wire Line
	3150 2100 3250 2100
$Comp
L Device:C_Small C11
U 1 1 5E0A72E2
P 2550 2350
F 0 "C11" H 2459 2304 50  0000 R CNN
F 1 "100nf" H 2459 2395 50  0000 R CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 2550 2350 50  0001 C CNN
F 3 "~" H 2550 2350 50  0001 C CNN
	1    2550 2350
	1    0    0    1   
$EndComp
Wire Wire Line
	2750 2500 2750 2550
Wire Wire Line
	2750 2550 2550 2550
Wire Wire Line
	2550 2550 2550 2450
Connection ~ 2750 2550
Wire Wire Line
	2750 2550 2750 2600
Wire Wire Line
	2550 2250 2550 2100
Wire Wire Line
	2550 2100 2500 2100
Wire Wire Line
	2550 2100 2750 2100
Connection ~ 2550 2100
$Comp
L Device:Q_PMOS_GDS Q1
U 1 1 5E07CD21
P 3050 1200
F 0 "Q1" V 3392 1200 50  0000 C CNN
F 1 "SUM110P06" V 3301 1200 50  0000 C CNN
F 2 "Package_TO_SOT_SMD:TO-263-2" H 3250 1300 50  0001 C CNN
F 3 "https://www.vishay.com/docs/72439/sum110p06-07l.pdf" H 3050 1200 50  0001 C CNN
	1    3050 1200
	0    -1   -1   0   
$EndComp
$Comp
L Device:D_Zener_ALT D2
U 1 1 5E0B4310
P 3350 1650
F 0 "D2" H 3250 1700 50  0000 C CNN
F 1 "Zener10V" H 3300 1550 50  0000 C CNN
F 2 "Diode_THT:D_DO-35_SOD27_P7.62mm_Horizontal" H 3350 1650 50  0001 C CNN
F 3 "~" H 3350 1650 50  0001 C CNN
	1    3350 1650
	-1   0    0    -1  
$EndComp
Wire Wire Line
	3200 1650 3150 1650
Wire Wire Line
	3150 1650 3150 1400
Connection ~ 3150 1400
Wire Wire Line
	3150 1400 3050 1400
Wire Wire Line
	3500 1650 3650 1650
Wire Wire Line
	3650 1650 3650 1400
Connection ~ 3650 1400
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	4050 3000 550  3000
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	550  500  4050 500 
Text Notes 600  2950 0    100  ~ 20
Power module (CAN0)
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	4050 3000 4050 500 
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	550  3000 550  500 
Connection ~ 3050 1400
$Comp
L Graphic:Logo_Open_Hardware_Small Open_Source_hardware1
U 1 1 5E0F6FA7
P 10650 6850
F 0 "Open_Source_hardware1" H 10650 7125 50  0001 C CNN
F 1 "Open Source Hardware" H 10650 6625 50  0000 C CNN
F 2 "Symbols:OSHW-Logo2_7.3x6mm_SilkScreen" H 10650 6850 50  0001 C CNN
F 3 "~" H 10650 6850 50  0001 C CNN
	1    10650 6850
	1    0    0    -1  
$EndComp
$Comp
L Morfeas_Rpi_Hat:SP3232 U4
U 1 1 5E056D92
P 9450 1700
F 0 "U4" H 9000 2300 50  0000 C CNN
F 1 "SP3232" H 9450 1950 50  0000 C CNN
F 2 "Package_SO:SOIC-16_3.9x9.9mm_P1.27mm" H 9450 1850 50  0001 L BNN
F 3 "1758642" H 9450 1850 50  0001 L BNN
F 4 "SP3232EET-L" H 9450 1850 50  0001 L BNN "Field4"
F 5 "SOIC-16" H 9450 1850 50  0001 L BNN "Field5"
F 6 "24R0382" H 9450 1850 50  0001 L BNN "Field6"
F 7 "Exar" H 9450 1850 50  0001 L BNN "Field7"
	1    9450 1700
	1    0    0    -1  
$EndComp
Wire Wire Line
	3850 6900 4000 6900
Connection ~ 4000 6900
$Comp
L power:+24V #PWR01
U 1 1 5E10CFE3
P 2100 6750
F 0 "#PWR01" H 2100 6600 50  0001 C CNN
F 1 "+24V" H 2115 6923 50  0000 C CNN
F 2 "" H 2100 6750 50  0001 C CNN
F 3 "" H 2100 6750 50  0001 C CNN
	1    2100 6750
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR013
U 1 1 5E10FCAE
P 3550 7200
F 0 "#PWR013" H 3550 6950 50  0001 C CNN
F 1 "GND" H 3555 7027 50  0000 C CNN
F 2 "" H 3550 7200 50  0001 C CNN
F 3 "" H 3550 7200 50  0001 C CNN
	1    3550 7200
	1    0    0    -1  
$EndComp
Wire Wire Line
	2450 6900 2600 6900
$Comp
L Device:C_Small C6
U 1 1 5E1216C5
P 5200 7100
F 0 "C6" H 5200 7200 50  0000 L CNN
F 1 "10uf" H 4950 7000 50  0000 L CNN
F 2 "Capacitor_SMD:C_1206_3216Metric" H 5200 7100 50  0001 C CNN
F 3 "~" H 5200 7100 50  0001 C CNN
	1    5200 7100
	-1   0    0    -1  
$EndComp
$Comp
L Device:C_Small C3
U 1 1 5E1225D4
P 4000 7100
F 0 "C3" H 4000 7200 50  0000 L CNN
F 1 "10uf" H 3750 7000 50  0000 L CNN
F 2 "Capacitor_SMD:C_1206_3216Metric" H 4000 7100 50  0001 C CNN
F 3 "~" H 4000 7100 50  0001 C CNN
	1    4000 7100
	-1   0    0    -1  
$EndComp
$Comp
L Device:C_Small C1
U 1 1 5E122C44
P 2600 7100
F 0 "C1" H 2600 7200 50  0000 L CNN
F 1 "10uF" H 2300 7000 50  0000 L CNN
F 2 "Capacitor_SMD:C_1206_3216Metric" H 2600 7100 50  0001 C CNN
F 3 "~" H 2600 7100 50  0001 C CNN
	1    2600 7100
	-1   0    0    -1  
$EndComp
$Comp
L Device:C_Small C2
U 1 1 5E124415
P 3100 7100
F 0 "C2" H 3100 7200 50  0000 L CNN
F 1 "10uf" H 3150 7000 50  0000 L CNN
F 2 "Capacitor_SMD:C_1206_3216Metric" H 3100 7100 50  0001 C CNN
F 3 "~" H 3100 7100 50  0001 C CNN
	1    3100 7100
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR07
U 1 1 5E124CE4
P 3100 7200
F 0 "#PWR07" H 3100 6950 50  0001 C CNN
F 1 "GND" H 3105 7027 50  0000 C CNN
F 2 "" H 3100 7200 50  0001 C CNN
F 3 "" H 3100 7200 50  0001 C CNN
	1    3100 7200
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR03
U 1 1 5E124F96
P 2600 7200
F 0 "#PWR03" H 2600 6950 50  0001 C CNN
F 1 "GND" H 2605 7027 50  0000 C CNN
F 2 "" H 2600 7200 50  0001 C CNN
F 3 "" H 2600 7200 50  0001 C CNN
	1    2600 7200
	1    0    0    -1  
$EndComp
Wire Wire Line
	2600 7000 2600 6900
Wire Wire Line
	3100 7000 3100 6900
Wire Wire Line
	3100 6900 3250 6900
Wire Wire Line
	4000 7000 4000 6900
Wire Wire Line
	5200 7000 5200 6900
Connection ~ 5200 6900
$Comp
L power:GND #PWR016
U 1 1 5E12EA67
P 4000 7200
F 0 "#PWR016" H 4000 6950 50  0001 C CNN
F 1 "GND" H 4005 7027 50  0000 C CNN
F 2 "" H 4000 7200 50  0001 C CNN
F 3 "" H 4000 7200 50  0001 C CNN
	1    4000 7200
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR022
U 1 1 5E12ED2F
P 5200 7200
F 0 "#PWR022" H 5200 6950 50  0001 C CNN
F 1 "GND" H 5205 7027 50  0000 C CNN
F 2 "" H 5200 7200 50  0001 C CNN
F 3 "" H 5200 7200 50  0001 C CNN
	1    5200 7200
	1    0    0    -1  
$EndComp
$Comp
L Device:L L1
U 1 1 5E134C9B
P 2850 6900
F 0 "L1" V 3040 6900 50  0000 C CNN
F 1 "10uH" V 2949 6900 50  0000 C CNN
F 2 "Resistors_SMD:R_2816_HandSoldering" H 2850 6900 50  0001 C CNN
F 3 "~" H 2850 6900 50  0001 C CNN
	1    2850 6900
	0    -1   -1   0   
$EndComp
Wire Wire Line
	2700 6900 2600 6900
Connection ~ 2600 6900
Wire Wire Line
	3000 6900 3100 6900
Connection ~ 3100 6900
$Comp
L Morfeas_Rpi_Hat:MAX9611AUB+ U7
U 1 1 5E14E76C
P 1850 4450
F 0 "U7" H 1550 4900 60  0000 C CNN
F 1 "MAX9611" H 1600 3950 60  0000 C CNN
F 2 "Morfeas_RPi_Hat:MAX9611AUB&plus_" H 2150 3800 60  0001 C CNN
F 3 "" H 750 4650 60  0000 C CNN
	1    1850 4450
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR033
U 1 1 5E14E772
P 1850 5200
F 0 "#PWR033" H 1850 4950 50  0001 C CNN
F 1 "GND" H 1855 5027 50  0000 C CNN
F 2 "" H 1850 5200 50  0001 C CNN
F 3 "" H 1850 5200 50  0001 C CNN
	1    1850 5200
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR035
U 1 1 5E14E778
P 2550 4050
F 0 "#PWR035" H 2550 3900 50  0001 C CNN
F 1 "+3.3V" H 2565 4223 50  0000 C CNN
F 2 "" H 2550 4050 50  0001 C CNN
F 3 "" H 2550 4050 50  0001 C CNN
	1    2550 4050
	1    0    0    -1  
$EndComp
Wire Wire Line
	2500 4300 2550 4300
$Comp
L Device:C_Small C14
U 1 1 5E14E77F
P 2700 4250
F 0 "C14" H 2700 4350 50  0000 L CNN
F 1 "100nf" H 2450 4150 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 2700 4250 50  0001 C CNN
F 3 "~" H 2700 4250 50  0001 C CNN
	1    2700 4250
	1    0    0    -1  
$EndComp
Connection ~ 2550 4100
Wire Wire Line
	2550 4100 2700 4100
Wire Wire Line
	2700 4100 2700 4150
Wire Wire Line
	2550 4100 2550 4050
Wire Wire Line
	2550 4100 2550 4300
$Comp
L power:GND #PWR037
U 1 1 5E14E78A
P 2700 4350
F 0 "#PWR037" H 2700 4100 50  0001 C CNN
F 1 "GND" H 2700 4200 50  0000 C CNN
F 2 "" H 2700 4350 50  0001 C CNN
F 3 "" H 2700 4350 50  0001 C CNN
	1    2700 4350
	1    0    0    -1  
$EndComp
$Comp
L Device:R_US R2
U 1 1 5E14E790
P 1850 3750
F 0 "R2" V 1645 3750 50  0000 C CNN
F 1 "22mΩ" V 1736 3750 50  0000 C CNN
F 2 "Resistor_SMD:R_2512_6332Metric" V 1890 3740 50  0001 C CNN
F 3 "~" H 1850 3750 50  0001 C CNN
	1    1850 3750
	0    1    1    0   
$EndComp
Wire Wire Line
	2050 3750 2000 3750
Wire Wire Line
	1650 3750 1700 3750
Text GLabel 1200 4650 0    50   Input ~ 0
SDA
Connection ~ 2050 3750
Wire Wire Line
	3650 3750 3650 3650
Wire Wire Line
	3250 3750 3650 3750
$Comp
L power:+24V #PWR043
U 1 1 5E14E79C
P 3650 3650
F 0 "#PWR043" H 3650 3500 50  0001 C CNN
F 1 "+24V" H 3665 3823 50  0000 C CNN
F 2 "" H 3650 3650 50  0001 C CNN
F 3 "" H 3650 3650 50  0001 C CNN
	1    3650 3650
	1    0    0    -1  
$EndComp
Wire Wire Line
	2850 3750 2050 3750
Connection ~ 1650 3750
Text GLabel 1200 4750 0    50   Input ~ 0
SCL
$Comp
L Device:R_US R10
U 1 1 5E14E7B2
P 3400 4050
F 0 "R10" V 3500 4050 50  0000 C CNN
F 1 "2.2kΩ" V 3300 4050 50  0000 C CNN
F 2 "Resistor_SMD:R_1206_3216Metric" V 3440 4040 50  0001 C CNN
F 3 "~" H 3400 4050 50  0001 C CNN
	1    3400 4050
	0    -1   -1   0   
$EndComp
$Comp
L Device:R_US R8
U 1 1 5E14E7B8
P 3050 4300
F 0 "R8" H 3150 4250 50  0000 C CNN
F 1 "1.2kΩ" H 3200 4350 50  0000 C CNN
F 2 "Resistor_SMD:R_1206_3216Metric" V 3090 4290 50  0001 C CNN
F 3 "~" H 3050 4300 50  0001 C CNN
	1    3050 4300
	-1   0    0    1   
$EndComp
Wire Wire Line
	3050 4150 3050 4050
Wire Wire Line
	3250 4050 3150 4050
Connection ~ 3650 3750
Wire Wire Line
	3550 4050 3650 4050
Wire Wire Line
	3650 4050 3650 3750
Wire Wire Line
	2500 4550 3050 4550
Wire Wire Line
	3050 4550 3050 4450
$Comp
L Device:R_US R6
U 1 1 5E14E7C5
P 3000 4750
F 0 "R6" V 3100 4750 50  0000 C CNN
F 1 "18kΩ" V 2900 4750 50  0000 C CNN
F 2 "Resistor_SMD:R_1206_3216Metric" V 3040 4740 50  0001 C CNN
F 3 "~" H 3000 4750 50  0001 C CNN
	1    3000 4750
	0    -1   -1   0   
$EndComp
$Comp
L Device:R_US R4
U 1 1 5E14E7CB
P 2750 5000
F 0 "R4" H 2850 4950 50  0000 C CNN
F 1 "1.3kΩ" H 2900 5050 50  0000 C CNN
F 2 "Resistor_SMD:R_1206_3216Metric" V 2790 4990 50  0001 C CNN
F 3 "~" H 2750 5000 50  0001 C CNN
	1    2750 5000
	1    0    0    1   
$EndComp
$Comp
L power:GND #PWR039
U 1 1 5E14E7D1
P 2750 5250
F 0 "#PWR039" H 2750 5000 50  0001 C CNN
F 1 "GND" H 2755 5077 50  0000 C CNN
F 2 "" H 2750 5250 50  0001 C CNN
F 3 "" H 2750 5250 50  0001 C CNN
	1    2750 5250
	1    0    0    -1  
$EndComp
Wire Wire Line
	2750 4850 2750 4750
Wire Wire Line
	2850 4750 2750 4750
Connection ~ 2750 4750
$Comp
L power:+3.3V #PWR041
U 1 1 5E14E7DA
P 3250 4750
F 0 "#PWR041" H 3250 4600 50  0001 C CNN
F 1 "+3.3V" V 3265 4878 50  0000 L CNN
F 2 "" H 3250 4750 50  0001 C CNN
F 3 "" H 3250 4750 50  0001 C CNN
	1    3250 4750
	0    1    1    0   
$EndComp
Wire Wire Line
	3150 4750 3250 4750
$Comp
L Device:C_Small C12
U 1 1 5E14E7E1
P 2550 5000
F 0 "C12" H 2459 4954 50  0000 R CNN
F 1 "100nf" H 2459 5045 50  0000 R CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 2550 5000 50  0001 C CNN
F 3 "~" H 2550 5000 50  0001 C CNN
	1    2550 5000
	1    0    0    1   
$EndComp
Wire Wire Line
	2750 5150 2750 5200
Wire Wire Line
	2750 5200 2550 5200
Wire Wire Line
	2550 5200 2550 5100
Connection ~ 2750 5200
Wire Wire Line
	2750 5200 2750 5250
Wire Wire Line
	2550 4900 2550 4750
Wire Wire Line
	2550 4750 2500 4750
Wire Wire Line
	2550 4750 2750 4750
Connection ~ 2550 4750
$Comp
L Device:Q_PMOS_GDS Q2
U 1 1 5E14E7F0
P 3050 3850
F 0 "Q2" V 3392 3850 50  0000 C CNN
F 1 "SUM110P06" V 3301 3850 50  0000 C CNN
F 2 "Package_TO_SOT_SMD:TO-263-2" H 3250 3950 50  0001 C CNN
F 3 "https://www.vishay.com/docs/72439/sum110p06-07l.pdf" H 3050 3850 50  0001 C CNN
	1    3050 3850
	0    -1   -1   0   
$EndComp
$Comp
L Device:D_Zener_ALT D3
U 1 1 5E14E7F6
P 3350 4300
F 0 "D3" H 3250 4350 50  0000 C CNN
F 1 "Zener10V" H 3300 4200 50  0000 C CNN
F 2 "Diode_THT:D_DO-35_SOD27_P7.62mm_Horizontal" H 3350 4300 50  0001 C CNN
F 3 "~" H 3350 4300 50  0001 C CNN
	1    3350 4300
	-1   0    0    -1  
$EndComp
Wire Wire Line
	3200 4300 3150 4300
Wire Wire Line
	3150 4300 3150 4050
Connection ~ 3150 4050
Wire Wire Line
	3150 4050 3050 4050
Wire Wire Line
	3500 4300 3650 4300
Wire Wire Line
	3650 4300 3650 4050
Connection ~ 3650 4050
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	4050 5650 550  5650
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	550  3150 4050 3150
Text Notes 600  5600 0    100  ~ 20
Power module (CAN1)
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	4050 5650 4050 3150
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	550  5650 550  3150
Connection ~ 3050 4050
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	5500 7700 1550 7700
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	1550 7700 1550 6450
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	1550 6450 5500 6450
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	3100 7500 1550 7500
Text Notes 1590 7660 0    100  ~ 20
Power Supply Unit
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	5500 6450 5500 7700
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	3100 7500 3100 7700
Wire Wire Line
	2000 6900 2100 6900
Connection ~ 2100 6900
Wire Wire Line
	2150 6900 2100 6900
Wire Wire Line
	2100 6750 2100 6900
$Comp
L power:GND #PWR02
U 1 1 5E1B512D
P 2100 7200
F 0 "#PWR02" H 2100 6950 50  0001 C CNN
F 1 "GND" H 2105 7027 50  0000 C CNN
F 2 "" H 2100 7200 50  0001 C CNN
F 3 "" H 2100 7200 50  0001 C CNN
	1    2100 7200
	1    0    0    -1  
$EndComp
Wire Wire Line
	2000 7100 2100 7100
Wire Wire Line
	2100 7100 2100 7200
$Comp
L power:+3.3V #PWR023
U 1 1 5E1C673A
P 9450 850
F 0 "#PWR023" H 9450 700 50  0001 C CNN
F 1 "+3.3V" H 9465 1023 50  0000 C CNN
F 2 "" H 9450 850 50  0001 C CNN
F 3 "" H 9450 850 50  0001 C CNN
	1    9450 850 
	1    0    0    -1  
$EndComp
Wire Wire Line
	9450 850  9450 900 
Wire Wire Line
	9550 900  9450 900 
Connection ~ 9450 900 
Wire Wire Line
	9450 900  9450 950 
$Comp
L Device:C_Small C7
U 1 1 5E1C6744
P 9650 900
F 0 "C7" V 9550 900 50  0000 C CNN
F 1 "100nf" V 9750 900 50  0000 C CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 9650 900 50  0001 C CNN
F 3 "~" H 9650 900 50  0001 C CNN
	1    9650 900 
	0    1    1    0   
$EndComp
$Comp
L power:GND #PWR025
U 1 1 5E1C674A
P 9750 900
F 0 "#PWR025" H 9750 650 50  0001 C CNN
F 1 "GND" H 9755 727 50  0000 C CNN
F 2 "" H 9750 900 50  0001 C CNN
F 3 "" H 9750 900 50  0001 C CNN
	1    9750 900 
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR024
U 1 1 5E1C9F3C
P 9450 2500
F 0 "#PWR024" H 9450 2250 50  0001 C CNN
F 1 "GND" H 9455 2327 50  0000 C CNN
F 2 "" H 9450 2500 50  0001 C CNN
F 3 "" H 9450 2500 50  0001 C CNN
	1    9450 2500
	1    0    0    -1  
$EndComp
$Comp
L Device:C_Small C9
U 1 1 5E1CBCD5
P 10250 1250
F 0 "C9" V 10150 1250 50  0000 C CNN
F 1 "100nf" V 10350 1250 50  0000 C CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 10250 1250 50  0001 C CNN
F 3 "~" H 10250 1250 50  0001 C CNN
	1    10250 1250
	0    1    1    0   
$EndComp
$Comp
L Device:C_Small C10
U 1 1 5E1CC0C6
P 10250 1550
F 0 "C10" V 10150 1550 50  0000 C CNN
F 1 "100nf" V 10350 1550 50  0000 C CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 10250 1550 50  0001 C CNN
F 3 "~" H 10250 1550 50  0001 C CNN
	1    10250 1550
	0    1    1    0   
$EndComp
$Comp
L Device:C_Small C4
U 1 1 5E1CC45D
P 8500 1250
F 0 "C4" V 8400 1250 50  0000 C CNN
F 1 "100nf" V 8600 1250 50  0000 C CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 8500 1250 50  0001 C CNN
F 3 "~" H 8500 1250 50  0001 C CNN
	1    8500 1250
	-1   0    0    1   
$EndComp
$Comp
L Device:C_Small C5
U 1 1 5E1D0DDE
P 8500 1550
F 0 "C5" V 8400 1550 50  0000 C CNN
F 1 "100nf" V 8600 1550 50  0000 C CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 8500 1550 50  0001 C CNN
F 3 "~" H 8500 1550 50  0001 C CNN
	1    8500 1550
	-1   0    0    1   
$EndComp
Wire Wire Line
	8750 1450 8500 1450
Wire Wire Line
	8500 1650 8700 1650
Wire Wire Line
	8700 1650 8700 1550
Wire Wire Line
	8700 1550 8750 1550
Wire Wire Line
	8750 1350 8500 1350
Wire Wire Line
	8750 1250 8700 1250
Wire Wire Line
	8700 1250 8700 1150
Wire Wire Line
	8700 1150 8500 1150
$Comp
L power:GND #PWR029
U 1 1 5E1E238A
P 10350 1250
F 0 "#PWR029" H 10350 1000 50  0001 C CNN
F 1 "GND" H 10355 1077 50  0000 C CNN
F 2 "" H 10350 1250 50  0001 C CNN
F 3 "" H 10350 1250 50  0001 C CNN
	1    10350 1250
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR030
U 1 1 5E1E291C
P 10350 1550
F 0 "#PWR030" H 10350 1300 50  0001 C CNN
F 1 "GND" H 10355 1377 50  0000 C CNN
F 2 "" H 10350 1550 50  0001 C CNN
F 3 "" H 10350 1550 50  0001 C CNN
	1    10350 1550
	0    -1   -1   0   
$EndComp
Wire Wire Line
	6550 3050 6650 3050
Wire Wire Line
	6650 3050 6650 2950
Wire Wire Line
	6550 2950 6650 2950
Connection ~ 6650 2950
Wire Wire Line
	6650 2950 6650 2850
$Comp
L power:GND #PWR09
U 1 1 5E1F9F06
P 6550 3550
F 0 "#PWR09" H 6550 3300 50  0001 C CNN
F 1 "GND" H 6555 3377 50  0000 C CNN
F 2 "" H 6550 3550 50  0001 C CNN
F 3 "" H 6550 3550 50  0001 C CNN
	1    6550 3550
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR010
U 1 1 5E1FA2B1
P 6550 3850
F 0 "#PWR010" H 6550 3600 50  0001 C CNN
F 1 "GND" H 6555 3677 50  0000 C CNN
F 2 "" H 6550 3850 50  0001 C CNN
F 3 "" H 6550 3850 50  0001 C CNN
	1    6550 3850
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR011
U 1 1 5E1FA583
P 6550 4350
F 0 "#PWR011" H 6550 4100 50  0001 C CNN
F 1 "GND" H 6555 4177 50  0000 C CNN
F 2 "" H 6550 4350 50  0001 C CNN
F 3 "" H 6550 4350 50  0001 C CNN
	1    6550 4350
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR012
U 1 1 5E1FA8B4
P 6550 4550
F 0 "#PWR012" H 6550 4300 50  0001 C CNN
F 1 "GND" H 6555 4377 50  0000 C CNN
F 2 "" H 6550 4550 50  0001 C CNN
F 3 "" H 6550 4550 50  0001 C CNN
	1    6550 4550
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR06
U 1 1 5E1FAC07
P 6050 4850
F 0 "#PWR06" H 6050 4600 50  0001 C CNN
F 1 "GND" H 6055 4677 50  0000 C CNN
F 2 "" H 6050 4850 50  0001 C CNN
F 3 "" H 6050 4850 50  0001 C CNN
	1    6050 4850
	0    1    1    0   
$EndComp
$Comp
L power:GND #PWR05
U 1 1 5E1FB16F
P 6050 4150
F 0 "#PWR05" H 6050 3900 50  0001 C CNN
F 1 "GND" H 6055 3977 50  0000 C CNN
F 2 "" H 6050 4150 50  0001 C CNN
F 3 "" H 6050 4150 50  0001 C CNN
	1    6050 4150
	0    1    1    0   
$EndComp
$Comp
L power:GND #PWR04
U 1 1 5E1FB65E
P 6050 3350
F 0 "#PWR04" H 6050 3100 50  0001 C CNN
F 1 "GND" H 6055 3177 50  0000 C CNN
F 2 "" H 6050 3350 50  0001 C CNN
F 3 "" H 6050 3350 50  0001 C CNN
	1    6050 3350
	0    1    1    0   
$EndComp
Text GLabel 5550 3050 0    50   Input ~ 0
SDA
Text GLabel 5550 3150 0    50   Input ~ 0
SCL
Wire Wire Line
	5550 3050 5650 3050
Wire Wire Line
	5550 3150 5850 3150
Text GLabel 6650 3350 2    50   Input ~ 0
RXD
Wire Wire Line
	6550 3250 6650 3250
Wire Wire Line
	6650 3350 6550 3350
Text GLabel 6650 3250 2    50   Input ~ 0
TXD
NoConn ~ 8900 3650
NoConn ~ 10100 4050
NoConn ~ 10100 3950
NoConn ~ 6050 2950
Text GLabel 8750 1750 0    50   Input ~ 0
TXD
Text GLabel 8750 2050 0    50   Input ~ 0
RXD
$Comp
L power:GND #PWR0101
U 1 1 5E228036
P 8700 1900
F 0 "#PWR0101" H 8700 1650 50  0001 C CNN
F 1 "GND" H 8705 1727 50  0000 C CNN
F 2 "" H 8700 1900 50  0001 C CNN
F 3 "" H 8700 1900 50  0001 C CNN
	1    8700 1900
	0    1    1    0   
$EndComp
Wire Wire Line
	8700 1900 8750 1900
Wire Wire Line
	8750 1900 8750 1850
NoConn ~ 10150 2150
NoConn ~ 10150 1850
NoConn ~ 8750 2150
$Comp
L Regulator_Linear:L7805 U2
U 1 1 5E106625
P 3550 6900
F 0 "U2" H 3400 7150 50  0000 C CNN
F 1 "TSR2-2450" H 3550 7051 50  0000 C CNN
F 2 "Morfeas_RPi_hat:TSR2-2450" H 3575 6750 50  0001 L CIN
F 3 "http://www.st.com/content/ccc/resource/technical/document/datasheet/41/4f/b3/b0/12/d4/47/88/CD00000444.pdf/files/CD00000444.pdf/jcr:content/translations/en.CD00000444.pdf" H 3550 6850 50  0001 C CNN
	1    3550 6900
	1    0    0    -1  
$EndComp
NoConn ~ 6050 3750
NoConn ~ 6050 3850
NoConn ~ 6050 3950
NoConn ~ 6050 4050
NoConn ~ 6550 4050
NoConn ~ 6550 4150
NoConn ~ 6050 4250
NoConn ~ 6550 4250
$Comp
L Mechanical:MountingHole H1
U 1 1 5E2EEC57
P 2050 5950
F 0 "H1" H 2150 5996 50  0000 L CNN
F 1 "MountingHole" H 2150 5905 50  0000 L CNN
F 2 "MountingHole:MountingHole_3mm" H 2050 5950 50  0001 C CNN
F 3 "~" H 2050 5950 50  0001 C CNN
	1    2050 5950
	1    0    0    -1  
$EndComp
$Comp
L Mechanical:MountingHole H2
U 1 1 5E2F00AC
P 2050 6150
F 0 "H2" H 2150 6196 50  0000 L CNN
F 1 "MountingHole" H 2150 6105 50  0000 L CNN
F 2 "MountingHole:MountingHole_3mm" H 2050 6150 50  0001 C CNN
F 3 "~" H 2050 6150 50  0001 C CNN
	1    2050 6150
	1    0    0    -1  
$EndComp
$Comp
L Mechanical:MountingHole H3
U 1 1 5E2F0330
P 2850 5950
F 0 "H3" H 2950 5996 50  0000 L CNN
F 1 "MountingHole" H 2950 5905 50  0000 L CNN
F 2 "MountingHole:MountingHole_3mm" H 2850 5950 50  0001 C CNN
F 3 "~" H 2850 5950 50  0001 C CNN
	1    2850 5950
	1    0    0    -1  
$EndComp
$Comp
L Mechanical:MountingHole H4
U 1 1 5E2F07C8
P 2850 6150
F 0 "H4" H 2950 6196 50  0000 L CNN
F 1 "MountingHole" H 2950 6105 50  0000 L CNN
F 2 "MountingHole:MountingHole_3mm" H 2850 6150 50  0001 C CNN
F 3 "~" H 2850 6150 50  0001 C CNN
	1    2850 6150
	1    0    0    -1  
$EndComp
$Comp
L Morfeas_Rpi_Hat:TLV70433DBVT U3
U 1 1 5E056599
P 4600 6950
F 0 "U3" H 4300 7250 60  0000 L CNN
F 1 "TLV70433" H 4300 7150 60  0000 L CNN
F 2 "Morfeas_RPi_hat:TLV70433DBVT" H 5100 6800 60  0001 C CNN
F 3 "" H 4000 7200 60  0000 C CNN
	1    4600 6950
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x03_Male J5
U 1 1 5E075351
P 10750 2050
F 0 "J5" H 10722 2074 50  0000 R CNN
F 1 "RS_232" H 10722 1983 50  0000 R CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical" H 10750 2050 50  0001 C CNN
F 3 "~" H 10750 2050 50  0001 C CNN
	1    10750 2050
	-1   0    0    -1  
$EndComp
Wire Wire Line
	10150 1750 10350 1750
Wire Wire Line
	10350 1750 10350 1950
Wire Wire Line
	10350 1950 10550 1950
Wire Wire Line
	10150 2050 10550 2050
Wire Wire Line
	10550 2150 10350 2150
Wire Wire Line
	10350 2150 10350 2250
$Comp
L power:GND #PWR044
U 1 1 5E093460
P 10350 2250
F 0 "#PWR044" H 10350 2000 50  0001 C CNN
F 1 "GND" H 10355 2077 50  0000 C CNN
F 2 "" H 10350 2250 50  0001 C CNN
F 3 "" H 10350 2250 50  0001 C CNN
	1    10350 2250
	1    0    0    -1  
$EndComp
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	9300 4800 11150 4800
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	11150 4800 11150 6450
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	11150 6450 9300 6450
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	9300 6450 9300 4800
Text Notes 9350 6400 0    100  ~ 20
Configuration EEPROM
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	11100 6200 9300 6200
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	8150 600  11150 600 
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	11150 2750 8150 2750
Text Notes 8150 2950 0    100  ~ 20
Level Translator (UART <-> RS-232)
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	11150 3000 8150 3000
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	8150 600  8150 3000
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	11150 600  11150 3000
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	8500 3150 11150 3150
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	11150 3150 11150 4650
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	11150 4650 8500 4650
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	8500 4650 8500 3150
Text Notes 8600 4600 0    100  ~ 20
Real Time Clock
$Comp
L Device:LED_CRGB D4
U 1 1 5E0A6AD9
P 8700 5700
F 0 "D4" H 8700 6197 50  0000 C CNN
F 1 "Status_LEDs" H 8700 6106 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical" H 8700 5650 50  0001 C CNN
F 3 "~" H 8700 5650 50  0001 C CNN
	1    8700 5700
	1    0    0    -1  
$EndComp
$Comp
L Device:R_US R11
U 1 1 5E0A9E2D
P 8300 5700
F 0 "R11" V 8400 5700 50  0000 C CNN
F 1 "330Ω" V 8200 5700 50  0000 C CNN
F 2 "Resistor_SMD:R_1206_3216Metric" V 8340 5690 50  0001 C CNN
F 3 "~" H 8300 5700 50  0001 C CNN
	1    8300 5700
	0    1    -1   0   
$EndComp
Wire Wire Line
	8450 5700 8500 5700
$Comp
L power:GND #PWR0102
U 1 1 5E0B2EA4
P 8150 5700
F 0 "#PWR0102" H 8150 5450 50  0001 C CNN
F 1 "GND" H 8155 5527 50  0000 C CNN
F 2 "" H 8150 5700 50  0001 C CNN
F 3 "" H 8150 5700 50  0001 C CNN
	1    8150 5700
	0    1    1    0   
$EndComp
$Comp
L power:+3.3V #PWR0103
U 1 1 5E09B957
P 1200 1750
F 0 "#PWR0103" H 1200 1600 50  0001 C CNN
F 1 "+3.3V" V 1200 2000 50  0000 C CNN
F 2 "" H 1200 1750 50  0001 C CNN
F 3 "" H 1200 1750 50  0001 C CNN
	1    1200 1750
	0    -1   -1   0   
$EndComp
$Comp
L power:+3.3V #PWR0104
U 1 1 5E09C9C1
P 1150 4350
F 0 "#PWR0104" H 1150 4200 50  0001 C CNN
F 1 "+3.3V" V 1150 4600 50  0000 C CNN
F 2 "" H 1150 4350 50  0001 C CNN
F 3 "" H 1150 4350 50  0001 C CNN
	1    1150 4350
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR0105
U 1 1 5E0A57CE
P 1200 1650
F 0 "#PWR0105" H 1200 1400 50  0001 C CNN
F 1 "GND" V 1200 1450 50  0000 C CNN
F 2 "" H 1200 1650 50  0001 C CNN
F 3 "" H 1200 1650 50  0001 C CNN
	1    1200 1650
	0    1    1    0   
$EndComp
Wire Wire Line
	1150 4350 1200 4350
Wire Wire Line
	1200 4350 1200 4300
Wire Wire Line
	1200 4400 1200 4350
Connection ~ 1200 4350
Text Notes 2800 2950 0    75   ~ 15
Addr:0x7C(1111100)
Text Notes 2800 5600 0    75   ~ 15
Addr:0x7F(1111111)
Text Notes 2600 2050 0    50   ~ 0
≈4A
Text Notes 2600 4700 0    50   ~ 0
≈4A
$Comp
L Device:C_Small C15
U 1 1 5E0ACB59
P 10400 3550
F 0 "C15" V 10300 3550 50  0000 C CNN
F 1 "100nf" V 10500 3550 50  0000 C CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 10400 3550 50  0001 C CNN
F 3 "~" H 10400 3550 50  0001 C CNN
	1    10400 3550
	0    1    1    0   
$EndComp
Wire Wire Line
	10300 3550 10200 3550
Connection ~ 10200 3550
Wire Wire Line
	10200 3550 10200 3500
$Comp
L power:+3.3V #PWR017
U 1 1 5E05BB44
P 10200 3500
F 0 "#PWR017" H 10200 3350 50  0001 C CNN
F 1 "+3.3V" H 10215 3673 50  0000 C CNN
F 2 "" H 10200 3500 50  0001 C CNN
F 3 "" H 10200 3500 50  0001 C CNN
	1    10200 3500
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR045
U 1 1 5E0C18FA
P 10550 3550
F 0 "#PWR045" H 10550 3300 50  0001 C CNN
F 1 "GND" H 10555 3377 50  0000 C CNN
F 2 "" H 10550 3550 50  0001 C CNN
F 3 "" H 10550 3550 50  0001 C CNN
	1    10550 3550
	0    -1   -1   0   
$EndComp
Wire Wire Line
	10500 3550 10550 3550
$Comp
L Device:R_US R12
U 1 1 5E26B6BA
P 5650 2800
F 0 "R12" H 5750 2650 50  0000 C CNN
F 1 "1kΩ" V 5750 2850 50  0000 C CNN
F 2 "Resistor_SMD:R_1206_3216Metric" V 5690 2790 50  0001 C CNN
F 3 "~" H 5650 2800 50  0001 C CNN
	1    5650 2800
	1    0    0    -1  
$EndComp
$Comp
L Device:R_US R13
U 1 1 5E27FB7B
P 5850 2800
F 0 "R13" H 5950 2650 50  0000 C CNN
F 1 "1kΩ" V 5950 2850 50  0000 C CNN
F 2 "Resistor_SMD:R_1206_3216Metric" V 5890 2790 50  0001 C CNN
F 3 "~" H 5850 2800 50  0001 C CNN
	1    5850 2800
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR046
U 1 1 5E288B85
P 5750 2550
F 0 "#PWR046" H 5750 2400 50  0001 C CNN
F 1 "+3.3V" H 5765 2723 50  0000 C CNN
F 2 "" H 5750 2550 50  0001 C CNN
F 3 "" H 5750 2550 50  0001 C CNN
	1    5750 2550
	1    0    0    -1  
$EndComp
Wire Wire Line
	5650 2650 5650 2600
Wire Wire Line
	5650 2600 5750 2600
Wire Wire Line
	5850 2600 5850 2650
Wire Wire Line
	5750 2600 5750 2550
Connection ~ 5750 2600
Wire Wire Line
	5750 2600 5850 2600
Wire Wire Line
	5650 2950 5650 3050
Connection ~ 5650 3050
Wire Wire Line
	5650 3050 6050 3050
Wire Wire Line
	5850 2950 5850 3150
Connection ~ 5850 3150
Wire Wire Line
	5850 3150 6050 3150
Text GLabel 6050 4350 0    50   Input ~ 0
R
Text GLabel 6050 4450 0    50   Input ~ 0
G
Text GLabel 6050 4550 0    50   Input ~ 0
B
Text GLabel 8900 5500 2    50   Input ~ 0
R
Text GLabel 8900 5700 2    50   Input ~ 0
G
Text GLabel 8900 5900 2    50   Input ~ 0
B
NoConn ~ 6550 4850
NoConn ~ 6550 4750
NoConn ~ 6550 4650
NoConn ~ 6550 4450
NoConn ~ 6050 4650
NoConn ~ 6050 4750
NoConn ~ 6550 3950
NoConn ~ 6550 3750
NoConn ~ 6550 3650
NoConn ~ 6050 3650
NoConn ~ 6050 3550
NoConn ~ 6050 3450
NoConn ~ 6550 3450
NoConn ~ 6050 3250
Wire Wire Line
	1650 3750 1300 3750
$Comp
L Device:D_ALT D6
U 1 1 5E0B78DF
P 1500 3400
F 0 "D6" H 1450 3450 50  0000 R CNN
F 1 "NRVTSA4100ET3G" H 1600 3300 50  0000 R CNN
F 2 "Morfeas_RPi_hat:DO-214AC(SMA)" H 1500 3400 50  0001 C CNN
F 3 "~" H 1500 3400 50  0001 C CNN
	1    1500 3400
	1    0    0    1   
$EndComp
$Comp
L power:GND #PWR048
U 1 1 5E12E002
P 1650 3450
F 0 "#PWR048" H 1650 3200 50  0001 C CNN
F 1 "GND" H 1655 3277 50  0000 C CNN
F 2 "" H 1650 3450 50  0001 C CNN
F 3 "" H 1650 3450 50  0001 C CNN
	1    1650 3450
	1    0    0    -1  
$EndComp
Wire Wire Line
	1650 3400 1650 3450
Wire Wire Line
	1350 3400 1300 3400
Wire Wire Line
	1300 3400 1300 3750
Connection ~ 1300 3750
$Comp
L Device:D_ALT D5
U 1 1 5E165919
P 1500 750
F 0 "D5" H 1450 800 50  0000 R CNN
F 1 "NRVTSA4100ET3G" H 1600 650 50  0000 R CNN
F 2 "Morfeas_RPi_hat:DO-214AC(SMA)" H 1500 750 50  0001 C CNN
F 3 "~" H 1500 750 50  0001 C CNN
	1    1500 750 
	1    0    0    1   
$EndComp
$Comp
L power:GND #PWR047
U 1 1 5E16591F
P 1650 800
F 0 "#PWR047" H 1650 550 50  0001 C CNN
F 1 "GND" H 1655 627 50  0000 C CNN
F 2 "" H 1650 800 50  0001 C CNN
F 3 "" H 1650 800 50  0001 C CNN
	1    1650 800 
	1    0    0    -1  
$EndComp
Wire Wire Line
	1650 750  1650 800 
Wire Wire Line
	1350 750  1300 750 
Wire Wire Line
	1300 750  1300 1100
Connection ~ 1300 1100
$Comp
L Morfeas_Rpi_Hat:Connector J1
U 7 1 5E1162EB
P 900 1100
F 0 "J1" H 792 915 50  0000 C CNN
F 1 "Con" H 792 1006 50  0001 C CNN
F 2 "Morfeas_RPi_hat:SPT1.5_8-H-3.5" H 900 1100 50  0001 C CNN
F 3 "" H 900 1100 50  0001 C CNN
	7    900  1100
	-1   0    0    1   
$EndComp
Wire Wire Line
	1100 1100 1300 1100
Wire Wire Line
	1150 1350 1300 1350
Wire Wire Line
	1300 1350 1300 1450
$Comp
L power:GND #PWR049
U 1 1 5E15111B
P 1300 1450
F 0 "#PWR049" H 1300 1200 50  0001 C CNN
F 1 "GND" H 1150 1400 50  0000 C CNN
F 2 "" H 1300 1450 50  0001 C CNN
F 3 "" H 1300 1450 50  0001 C CNN
	1    1300 1450
	1    0    0    -1  
$EndComp
Wire Wire Line
	1100 3750 1300 3750
$Comp
L Morfeas_Rpi_Hat:Connector J1
U 1 1 5E16FE86
P 900 3750
F 0 "J1" H 792 3565 50  0000 C CNN
F 1 "Con" H 792 3656 50  0000 C CNN
F 2 "Morfeas_RPi_hat:SPT1.5_8-H-3.5" H 900 3750 50  0001 C CNN
F 3 "" H 900 3750 50  0001 C CNN
	1    900  3750
	-1   0    0    1   
$EndComp
$Comp
L Morfeas_Rpi_Hat:Connector J1
U 5 1 5E171471
P 1800 6900
F 0 "J1" H 1650 6800 50  0000 C CNN
F 1 "Con" H 1900 6900 50  0001 C CNN
F 2 "Morfeas_RPi_hat:SPT1.5_8-H-3.5" H 1800 6900 50  0001 C CNN
F 3 "" H 1800 6900 50  0001 C CNN
	5    1800 6900
	-1   0    0    1   
$EndComp
$Comp
L Morfeas_Rpi_Hat:Connector J1
U 6 1 5E172C37
P 1800 7100
F 0 "J1" H 1650 7000 50  0000 C CNN
F 1 "Con" H 1900 7100 50  0001 C CNN
F 2 "Morfeas_RPi_hat:SPT1.5_8-H-3.5" H 1800 7100 50  0001 C CNN
F 3 "" H 1800 7100 50  0001 C CNN
	6    1800 7100
	-1   0    0    1   
$EndComp
$Comp
L power:GND #PWR0106
U 1 1 5E1939E0
P 1300 6200
F 0 "#PWR0106" H 1300 5950 50  0001 C CNN
F 1 "GND" H 1305 6027 50  0000 C CNN
F 2 "" H 1300 6200 50  0001 C CNN
F 3 "" H 1300 6200 50  0001 C CNN
	1    1300 6200
	1    0    0    -1  
$EndComp
Wire Wire Line
	1300 6150 1300 6200
Wire Wire Line
	1300 6150 1300 6050
Connection ~ 1300 6150
Connection ~ 1300 5950
Wire Wire Line
	1300 5950 1300 5850
Connection ~ 1300 6050
Wire Wire Line
	1300 6050 1300 5950
Wire Wire Line
	1200 6150 1300 6150
Wire Wire Line
	1200 6050 1300 6050
Wire Wire Line
	1200 5950 1300 5950
Wire Wire Line
	1300 5850 1200 5850
$Comp
L Morfeas_Rpi_Hat:Connector J1
U 8 1 5E1755AF
P 1000 5850
F 0 "J1" H 1050 5850 50  0000 L CNN
F 1 "Con" V 1050 5650 50  0001 L CNN
F 2 "Morfeas_RPi_hat:SPT1.5_8-H-3.5" H 1000 5850 50  0001 C CNN
F 3 "" H 1000 5850 50  0001 C CNN
	8    1000 5850
	-1   0    0    -1  
$EndComp
$Comp
L Morfeas_Rpi_Hat:Connector J1
U 4 1 5E174B4D
P 1000 5950
F 0 "J1" H 1050 5950 50  0000 L CNN
F 1 "Con" V 1050 5750 50  0001 L CNN
F 2 "Morfeas_RPi_hat:SPT1.5_8-H-3.5" H 1000 5950 50  0001 C CNN
F 3 "" H 1000 5950 50  0001 C CNN
	4    1000 5950
	-1   0    0    -1  
$EndComp
$Comp
L Morfeas_Rpi_Hat:Connector J1
U 3 1 5E1737B5
P 1000 6050
F 0 "J1" H 1050 6050 50  0000 L CNN
F 1 "Con" V 1050 5850 50  0001 L CNN
F 2 "Morfeas_RPi_hat:SPT1.5_8-H-3.5" H 1000 6050 50  0001 C CNN
F 3 "" H 1000 6050 50  0001 C CNN
	3    1000 6050
	-1   0    0    -1  
$EndComp
$Comp
L Morfeas_Rpi_Hat:Connector J1
U 2 1 5E12235A
P 1000 6150
F 0 "J1" H 1050 6150 50  0000 L CNN
F 1 "Con" V 1050 5950 50  0001 L CNN
F 2 "Morfeas_RPi_hat:SPT1.5_8-H-3.5" H 1000 6150 50  0001 C CNN
F 3 "" H 1000 6150 50  0001 C CNN
	2    1000 6150
	-1   0    0    -1  
$EndComp
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	5250 2250 7000 2250
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	7000 5200 5250 5200
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	7000 2250 7000 5200
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	5250 5200 5250 2250
Text Notes 5300 5150 0    75   ~ 15
RPi Header
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	6000 5000 6000 5200
Wire Notes Line width 20 style dash_dot rgb(255, 0, 0)
	6000 5000 5250 5000
$EndSCHEMATC
