/*
 * KEYPAD AND SEVEN-SEGEMENT DECODER IMPLEMENTATION FOR PART-1 LAB-1
 * The code will read the key pressed from the PMOD kypd module and the corresponding value will be displayed on both the SSDs (i.e., left as well as right)
 * Initially, the delay of 250 ms is used between left and right segment switching so user can clearly see that both segment do not appear to lit at the same time.
 * If user decreases this time to say, 10 ms, it will appear as if both the left and right segments lits at the same time.
 *
 *  ECE- 315 WINTER 2021 - COMPUTER INTERFACING COURSE
 *
 *  Created on: 	Date:::5 February, 2021
 *      Author: 	Shyama M. Gandhi
 */

//Include FreeRTOS Library
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "xparameters.h"
#include "xgpio.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "xil_printf.h"

#include "pmodkypd.h"
#include "sleep.h"
#include "xil_cache.h"

// Parameter definitions
#define SSD_DEVICE_ID		XPAR_AXI_GPIO_PMOD_SSD_DEVICE_ID
#define KYPD_DEVICE_ID		XPAR_AXI_GPIO_PMOD_KEYPAD_DEVICE_ID

//Button Variable
XGpio SSDInst, KYPDInst;

/* The Tx described at the top of this file. */
static void prvTxTask( void *pvParameters );

void DemoInitialize();

u32 SSD_decode(u8 key_value,u8 cathode);

PmodKYPD myDevice;

/*-----------------------------------------------------------*/

static TaskHandle_t xTxTask;

// keytable is determined as follows (indices shown in Keypad position below)
// 12 13 14 15
// 8  9  10 11
// 4  5  6  7
// 0  1  2  3

#define DEFAULT_KEYTABLE "0FED789C456B123A"

void DemoInitialize() {
   KYPD_begin(&myDevice, XPAR_AXI_GPIO_PMOD_KEYPAD_BASEADDR);
   KYPD_loadKeyTable(&myDevice, (u8*) DEFAULT_KEYTABLE);
}


u32 SSD_decode(u8 key_value, u8 cathode){

	switch(key_value){
	case 48: if(cathode==0) return 0b00111111; else return 0b10111111;
	case 49: if(cathode==0) return 0b00000110; else return 0b10000110;
	case 50: if(cathode==0) return 0b01011011; else return 0b11011011;
	case 51: if(cathode==0) return 0b01001111; else return 0b11001111;
	case 52: if(cathode==0) return 0b01100110; else return 0b11100110;
	case 53: if(cathode==0) return 0b01101101; else return 0b11101101;
	case 54: if(cathode==0) return 0b01111101; else return 0b11111101;
	case 55: if(cathode==0) return 0b00000111; else return 0b10000111;
	case 56: if(cathode==0) return 0b01111111; else return 0b11111111;
	case 57: if(cathode==0) return 0b01101111; else return 0b11101111;
	case 65: if(cathode==0) return 0b01110111; else return 0b11110111;
	case 66: if(cathode==0) return 0b01111100; else return 0b11111100;
	case 67: if(cathode==0) return 0b00111001; else return 0b10111001;
	case 68: if(cathode==0) return 0b01011110; else return 0b11011110;
	case 69: if(cathode==0) return 0b01111001; else return 0b11111001;
	case 70: if(cathode==0) return 0b01110001; else return 0b11110001;
	default: if(cathode==0) return 0b00000000; else return 0b00000000;
	}

}

//----------------------------------------------------
// MAIN FUNCTION
//----------------------------------------------------
int main (void)
{
  int status;
  //----------------------------------------------------
  // INITIALIZE THE PERIPHERALS & SET DIRECTIONS OF GPIO
  //----------------------------------------------------

  // Initialize SSD
  status = XGpio_Initialize(&SSDInst, SSD_DEVICE_ID);
  if(status != XST_SUCCESS){
    xil_printf("GPIO Initialization for SSD unsuccessful.\r\n");
    return XST_FAILURE;
  }

  // Set SSD direction to output
  XGpio_SetDataDirection(&SSDInst, 1, 0x00);

  xil_printf("Initialization Complete, System Ready!\n");

  /* Create the two tasks.  The Tx task is given a lower priority than the
  Rx task, so the Rx task will leave the Blocked state and pre-empt the Tx
  task as soon as the Tx task places an item in the queue. */
  xTaskCreate( prvTxTask,					/* The function that implements the task. */
    			( const char * ) "Tx", 		/* Text name for the task, provided to assist debugging only. */
    			configMINIMAL_STACK_SIZE, 	/* The stack allocated to the task. */
    			NULL, 						/* The task parameter is not used, so set to NULL. */
    			tskIDLE_PRIORITY,			/* The task runs at the idle priority. */
    			&xTxTask );

  DemoInitialize();

  vTaskStartScheduler();

  while(1);

  return 0;
}

/*-----------------------------------------------------------*/
static void prvTxTask( void *pvParameters )
{
	for( ;; ) {
	   u16 keystate;
	   XStatus status, last_status = KYPD_NO_KEY;
	   u8 key, last_key = 'x';
	   u32 ssd_value=0;


	   // Initial value of last_key cannot be contained in loaded KEYTABLE string
	   Xil_Out32(myDevice.GPIO_addr, 0xF);

	   xil_printf("Pmod KYPD demo started. Press any key on the Keypad.\r\n");
	   while (1) {
	      // Capture state of each key
	      keystate = KYPD_getKeyStates(&myDevice);

	      // Determine which single key is pressed, if any
	      status = KYPD_getKeyPressed(&myDevice, keystate, &key);

	      // Print key detect if a new key is pressed or if status has changed
	      if (status == KYPD_SINGLE_KEY
	            && (status != last_status || key != last_key)) {
	         xil_printf("Key Pressed: %c\r\n", (char) key);
	         last_key = key;

	      }
	      else if (status == KYPD_MULTI_KEY && status != last_status)
	         xil_printf("Error: Multiple keys pressed\r\n");

	      last_status = status;

	      XGpio_DiscreteWrite(&SSDInst, 1, 0b10000000);
	      ssd_value = SSD_decode(key, 0);
	      XGpio_DiscreteWrite(&SSDInst, 1, ssd_value );

	  	  vTaskDelay(pdMS_TO_TICKS(10));

	  	  XGpio_DiscreteWrite(&SSDInst, 1, 0b00000000);
	      ssd_value = SSD_decode(key, 1);
	  	  XGpio_DiscreteWrite(&SSDInst, 1, ssd_value);

	  	  vTaskDelay(pdMS_TO_TICKS(10));
	   }
	}
}
/*-----------------------------------------------------------*/
