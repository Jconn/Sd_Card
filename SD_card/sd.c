// sd.c
//
//****************************************************************************************************
// Author:
// 	Some Dude
//
// Credits:
//	Modified from countless TivaWare programs
//
// Requirements:
// 	Requires Texas Instruments' TivaWare.
//	Also requires FatFS, an SD library from ChaN
//
// Description:
//	Interface Tiva with SD Card
//
// Notes:
//	Sample Program for MAAV. Functionality described right in front of main
//
//****************************************************************************************************
#define TARGET_IS_TM4C123_RA1

// Includes ------------------------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"

#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"

#include "utils/uartstdio.h"

#include "ff.h"
#include "diskio.h"


// Defines -------------------------------------------------------------------------------------------
#define LED_RED GPIO_PIN_1
#define LED_BLUE GPIO_PIN_2
#define LED_GREEN GPIO_PIN_3




// Variables -----------------------------------------------------------------------------------------
FATFS sdVolume;			// FatFs work area needed for each volume
FIL logfile;			// File object needed for each open file
uint16_t fp;			// Used for sizeof




// Functions -----------------------------------------------------------------------------------------
void ConfigureUART(void){

	// Enable the peripherals used by UART
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

	// Set GPIO A0 and A1 as UART pins.
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

        // Configure UART clock using UART utils
        UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
        UARTStdioConfig(0, 115200, 16000000);
}


void FloatToPrint(float floatValue, uint32_t splitValue[2]){
	int32_t i32IntegerPart;
	int32_t i32FractionPart;

        i32IntegerPart = (int32_t) floatValue;
        i32FractionPart = (int32_t) (floatValue * 1000.0f);
        i32FractionPart = i32FractionPart - (i32IntegerPart * 1000);
        if(i32FractionPart < 0)
        {
            i32FractionPart *= -1;
        }

	splitValue[0] = i32IntegerPart;
	splitValue[1] = i32FractionPart;
}

void fatalError(char errMessage[]){
	UARTprintf(errMessage);
	ROM_GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_RED, LED_RED);
	while(1);
}



// Main ----------------------------------------------------------------------------------------------
/*
 * This Sample Program should do four things:
 * 	Creates a text file in an sd card or microsd card titled "jason.txt"
 * 	Writes the message "Sample text for MAAV, controls." onto "jason.txt"
 * 	Reads up to 400 bytes on "jason.txt" and replaces 'o' with 'X'
 * 	Writes the edited message onto "jason.txt"
 *
 * All platfrom specific code is stuck in "diskio.c"
 * SD card interfacing is achieved with  the FatFs library.
 * Documentation can be found at:
 * 	http://elm-chan.org/fsw/ff/00index_e.html
 * The documentation is very good, and will help you
 * understand how to use these functions.
 * All c files in this project besides
 * this one and diskio.c are source code for the library.
 *
 * 	major functions used are:
 * 		f_mount - mounts the sd card
 * 		f_open  - opens file on sd card, with read and/or write capabilities
 * 		f_read  - reads bytes from designated file
 * 		f_write - writes bytes to designated file.
 *
 * 	pins assigments for this demo can be found in "diskio.c"
 * 	The important assignments are as follows:
	  	#define SDC_GPIO_PORT_BASE      GPIO_PORTA_BASE
		#define SDC_GPIO_SYSCTL_PERIPH  SYSCTL_PERIPH_GPIOA
		#define SDC_SSI_TX              GPIO_PIN_5
		#define SDC_SSI_RX              GPIO_PIN_4
		#define SDC_SSI_FSS             GPIO_PIN_3
		#define SDC_SSI_CLK             GPIO_PIN_2

	This *should* work out of the box. On Success, an LED will turn green.
	If an LED turns any other color, there is a failure somewhere.
	RED - failure to mount
	YELLOW - failure to read from file
	BLUE - failure to open file
	If it does not, check your stack size.
	This can be done By going to Properties->CCS Build->ARM Linker->Basic Options.
	The stack size I have set is 2^14, it should work with 2^11 bytes allocated
 */
int main(void){

	// Enable lazy stacking
	ROM_FPULazyStackingEnable();

	// Set the system clock to run at 40Mhz off PLL with external crystal as reference.
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);
	//using 20 sets the clock to 10Mhz
	// Initialize the UART and write status.
	ConfigureUART();
	UARTprintf("SD Example\n");

	// Enable LEDs
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, LED_RED|LED_BLUE|LED_GREEN);
	ROM_GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_BLUE, 0);

	// Start SD Card Stuff - Borrowed from examples

	// Initialize result variable
	UINT bw;
	bw = 0;
	FRESULT res;
	// Mount the SD Card
	switch(f_mount(&sdVolume, "", 1)){
		case FR_OK:
			UARTprintf("SD Card mounted successfully\n");
			break;
		case FR_INVALID_DRIVE:
			fatalError("ERROR: Invalid drive number\n");
			break;
		case FR_DISK_ERR:
			fatalError("ERROR: DiskIO error - Check hardware!\n");
			break;
		case FR_NOT_READY:
			ROM_GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_BLUE, LED_RED);
			fatalError("ERROR: Medium removal or disk_initialize\n");
			break;
		case FR_NO_FILESYSTEM:
			fatalError("ERROR: No valid FAT volume on drive\n");
			break;
		default:
			fatalError("ERROR: Something went wrong\n");
			break;
	}
	char in_buff[400];
	UINT bytes_read;
	bytes_read = 400;
/*
 * Sample Write for initial test
 */
	char *sample_text = "Sample text for MAAV, controls.";
	if(f_open(&logfile, "jason.txt", FA_WRITE | FA_OPEN_ALWAYS) == FR_OK) {
		f_write(&logfile, (void*) sample_text,strlen(sample_text), &bw);						// Close the file
		f_close(&logfile);
		}
	else {
		ROM_GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_BLUE, LED_BLUE);
		while(1);
	}
//reads from "jason.txt"
if(f_open(&logfile, "jason.txt", FA_READ | FA_OPEN_EXISTING) == FR_OK) {
	res = f_read(&logfile, in_buff, bytes_read,&bytes_read );
	// failure conditional - lights LED Yellow
	if(res) {
		GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_BLUE, LED_GREEN|LED_RED);
		while(1);
	}
	f_close(&logfile);
	}
else {
	GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_BLUE, LED_BLUE|LED_RED);
	while(1);
}
	unsigned int i = 0;
	while (i < bytes_read) {
		if(in_buff[i] == 'o' || in_buff[i] == 'e')
			in_buff[i] = 'X';
		++i;
	}



if(f_open(&logfile, "jason.txt", FA_WRITE | FA_OPEN_ALWAYS) == FR_OK) {
	f_write(&logfile, (void*) in_buff,bytes_read, &bw);						// Close the file
	f_close(&logfile);
	}
//failure condition - writes LED blue
else {
	ROM_GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_BLUE, LED_BLUE);
	while(1);
}

 // Lights green LED if data written well
if (bw == bytes_read) {
	ROM_GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_BLUE, LED_GREEN);
}
	while(1);
	// Wait Forever



}
