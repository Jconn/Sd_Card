// sd.c
//
//****************************************************************************************************
// Author:
// 	Nipun Gunawardena
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
//	
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
			//fatalError("ERROR: Medium removal or disk_initialize\n");
			while(1);
			break;
		case FR_NO_FILESYSTEM:
			fatalError("ERROR: No valid FAT volume on drive\n");
			break;
		default:
			fatalError("ERROR: Something went wrong\n");
			break;
	}
	char in_buff[6000];
	UINT bytes_read;
	bytes_read = 6000;
//	char garbo_buf[400];
//	garbo_buf[0] ='a';
/*
if(f_open(&logfile, "jason.txt", FA_WRITE | FA_OPEN_ALWAYS) == FR_OK) {	// Open file - If nonexistent, create
		f_lseek(&logfile, logfile.fsize);					// Move forward by filesize; logfile.fsize+1 is not needed in this application
		f_write(&logfile, "Parachutes\n", 11, &bw);				// Append word
		f_lseek(&logfile,0);
		UARTprintf("File size is %u\n",logfile.fsize);				// Print size
		f_close(&logfile);							// Close the file
		if (bw == 11) {
			ROM_GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_BLUE, LED_GREEN); // Lights green LED if data written well
		}
	}
*/
if(f_open(&logfile, "jason.txt", FA_READ | FA_OPEN_EXISTING) == FR_OK) {
//	f_lseek(&logfile, 0);
	res = f_read(&logfile, in_buff, bytes_read,&bytes_read );
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
//	f_lseek(&logfile, logfile.fsize);
	//char read_msg[100];
	//uint16_t rm_len;
	//rm_len=sprintf (read_msg, "Succesfully read %u bytes", bytes_read);
	//f_write(&logfile, (void*)read_msg, rm_len, &bw);
	//rm_len = sprintf(read_msg, "Recreating read message\n");
	//f_write(&logfile, (void*)read_msg, rm_len, &bw);
	f_write(&logfile, (void*) in_buff,bytes_read, &bw);						// Close the file
	f_close(&logfile);
	}
	else {
		ROM_GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_BLUE, LED_BLUE);
		while(1);
	}
/*
	char *file_N = "jason.txt";
	char *char_Test = "It works James";
	char in_buff[200];
	UINT bytes_read;

	bytes_read = 10;
	if(f_open(&logfile, "jason.txt", FA_READ|FA_WRITE | FA_OPEN_ALWAYS) == FR_OK) {// Open file - If nonexistent, create
		f_lseek(&logfile, 0);					// Move forward by filesize; logfile.fsize+1 is not needed in this application
		//f_write(&logfile, (void*)char_Test, strlen(char_Test), &bw);				// Append word
		res = f_read(&logfile, in_buff, bytes_read,&bytes_read );
		if(res)
			while(1);
		unsigned int i = 0;
		while (i < bytes_read) {
			if(in_buff[i] == 'o')
				in_buff[i] = 'A';
			++i;
		}
		f_lseek(&logfile, logfile.fsize);
		char read_msg[100];
		uint16_t rm_len;
		rm_len=sprintf (read_msg, "Succesfully read %u bytes", bytes_read);
		f_write(&logfile, (void*)read_msg, rm_len, &bw);
		rm_len = sprintf(read_msg, "Recreating read message\n");
		f_write(&logfile, (void*)read_msg, rm_len, &bw);
		f_write(&logfile, (void*) in_buff,bytes_read, &bw);						// Close the file
		if (bw == strlen(char_Test)) {
			ROM_GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_BLUE, LED_GREEN); // Lights green LED if data written well
		}
		f_close(&logfile);
	}
	else
		bw = 7;
	// Wait Forever

*/
if (bw == bytes_read) {
	ROM_GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_BLUE, LED_GREEN); // Lights green LED if data written well
}
	while(1);
	// Wait Forever



}
