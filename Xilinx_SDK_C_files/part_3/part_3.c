/*
 * IMPLEMENTATION OF A SIMPLE CALCULATOR.
 * Inputs Operands from the keypad
 * Output of the arithmetic operation is displayed on the Console
 * Operations available : +, -, * and factorial, selected using the keys A, B, C and D
 *
 *  ECE- 315 WINTER 2021 - COMPUTER INTERFACING COURSE
 *
 *  Created on: 	Date:::6 February, 2021
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
#include "math.h"

/* The Tx and Rx tasks as described at the top of this file. */
static void prvTxTask( void *pvParameters );
static void prvRxTask( void *pvParameters );

u32 factorial(int n1);


void DemoInitialize();
u32 SSD_decode(u8 key_value, u8 cathode);

PmodKYPD myDevice;

/*-----------------------------------------------------------*/
/* The queue used by the Tx and Rx tasks, as described at the top of this
file. */
static TaskHandle_t xTxTask;
static TaskHandle_t xRxTask;
static QueueHandle_t xQueue = NULL;

#define DEFAULT_KEYTABLE "0FED789C456B123A"

void DemoInitialize() {
   KYPD_begin(&myDevice, XPAR_AXI_GPIO_PMOD_KEYPAD_BASEADDR);
   KYPD_loadKeyTable(&myDevice, (u8*) DEFAULT_KEYTABLE);
}

//----------------------------------------------------
// MAIN FUNCTION
//----------------------------------------------------
int main (void)
{
  int status;

  xil_printf("System Ready!\n");

  /* Create the two tasks.  The Tx task is given a lower priority than the
  Rx task, so the Rx task will leave the Blocked state and pre-empt the Tx
  task as soon as the Tx task places an item in the queue. */
  xTaskCreate( prvTxTask,					/* The function that implements the task. */
    			( const char * ) "Tx", 		/* Text name for the task, provided to assist debugging only. */
    			configMINIMAL_STACK_SIZE, 	/* The stack allocated to the task. */
    			NULL, 						/* The task parameter is not used, so set to NULL. */
    			tskIDLE_PRIORITY+2,			/* The task runs at the idle priority. */
    			&xTxTask );

  xTaskCreate( prvRxTask,
    			( const char * ) "Rx",
				configMINIMAL_STACK_SIZE,
				NULL,
    			tskIDLE_PRIORITY + 1,
    			&xRxTask );

  /* Create the queue used by the tasks.  The Rx task has a higher priority
  than the Tx task, so will preempt the Tx task and remove values from the
  queue as soon as the Tx task writes to the queue - therefore the queue can
  never have more than one item in it. */
  xQueue = xQueueCreate( 3,				/* There is only one space in the queue. */
		  	  sizeof( unsigned int ) );	/* Each space in the queue is large enough to hold a uint32_t. */

  /* Check the queue was created. */
  configASSERT(xQueue);

  DemoInitialize();

  vTaskStartScheduler();

  while(1);

  return 0;
}

/*-----------------------------------------------------------*/
static void prvTxTask( void *pvParameters )
{
	UBaseType_t uxPriority;

	for( ;; ) {
	   u16 keystate;
	   XStatus status, last_status = KYPD_NO_KEY;
	   u8 key, last_key = 'x';

	   u32 factor = 0, current_value = 0;

	   // Initial value of last_key cannot be contained in loaded KEYTABLE string
	   Xil_Out32(myDevice.GPIO_addr, 0xF);

	   xil_printf("Pmod KYPD demo started. Press any key on the Keypad.\r\n");

	   uxPriority = uxTaskPriorityGet( NULL );

	   while (1) {

	      // Capture state of each key
	      keystate = KYPD_getKeyStates(&myDevice);

	      // Determine which single key is pressed, if any
	      status = KYPD_getKeyPressed(&myDevice, keystate, &key);

	      //this functions returns the number of entries in the queue, so when the queue is full, i.e., 2 entries, decreased the priority of this task and hence recieve task
	      //will immediately start to execute.
	      if(uxQueueMessagesWaiting( xQueue ) == 3){
	    	  xil_printf("Going to task 2!\n");
	   		  vTaskPrioritySet( NULL, ( uxPriority - 2 ) );
	      }

	      // Print key detect if a new key is pressed or if status has changed
	      if (status == KYPD_SINGLE_KEY
	            && (status != last_status || key != last_key)) {
//	         xil_printf("Key Pressed: %c\r\n", (char) key);
	         last_key = key;


	         if((char)key == 'F'){
	        	 current_value = current_value*10 + factor;
	        	 xil_printf("final current_value of operand= %d\n",current_value);

		         xQueueSend( xQueue,			/* The queue being written to. */
		         			 &current_value,    /* The address of the data being sent. */
		         			 0UL );				/* The block time. */

		         current_value = 0;
	         }
	         //case when we consider input key strokes from '0' to '9' (only these are the valid key inputs for arithmetic operation
	         else if((char)key == 'E' ){
	        	 current_value = current_value*10 + factor;
	        	 xil_printf("current_value = %d\n",current_value);
	         }
	         //if user presses 'E' key, consider the last input key pressed as the operand
	         else if((int)key>=48 && (int)key<=57){
	        	 factor = (int)key - 48;
	         }
	         else if((uxQueueMessagesWaiting( xQueue ) == 2) && ((char)key == 'A' || (char)key == 'B' || (char)key == 'C' || (char)key == 'D')){
	        	 current_value = (int)key;
		         xQueueSend( xQueue,			/* The queue being written to. */
		         			 &current_value,    			/* The address of the data being sent. */
		         			 0UL );				/* The block time. */
		         current_value = 0;

	         }
	      }
	      else if (status == KYPD_MULTI_KEY && status != last_status)
	         xil_printf("Error: Multiple keys pressed\r\n"); //this is valid whenever two or more keys are pressed together

	      last_status = status;
	      usleep(1000);
	   }
	}
}
/*-----------------------------------------------------------*/
static void prvRxTask( void *pvParameters )
{
	UBaseType_t uxPriority;
	uxPriority = uxTaskPriorityGet( NULL );

	for( ;; )
	{
		u32 read_queue_value;
		u32 store_operands[2];
		u32 operation_to_be_done=0;
		u32 result=0; //"int" because we may get negative results in subtraction.

		while(uxQueueMessagesWaiting(xQueue)!=0){
//			xil_printf("queue items = %llU\n",uxQueueMessagesWaiting(xQueue));

			/* Block to wait for data arriving on the queue. */
			xQueueReceive( 	xQueue,				/* The queue being read. */
							&read_queue_value,	/* Data is read into this address. */
							portMAX_DELAY );	/* Wait without a timeout for data. */

			xil_printf("Received value :: %d\n",read_queue_value);

//			//in total three values will be read from the queue so, store them in arrays as two operands and the last value read from the queue will be the operation indicator.
			if(uxQueueMessagesWaiting(xQueue)==2)
				store_operands[0]=read_queue_value;
			else if(uxQueueMessagesWaiting(xQueue)==1)
				store_operands[1]=read_queue_value;
			else if(uxQueueMessagesWaiting(xQueue)==0)
				operation_to_be_done=read_queue_value;

		}

		xil_printf("store_operands[0]=%d\nstore_operands[1]=%d\noperation_to_be_done=%d\n", store_operands[0],store_operands[1],operation_to_be_done);

		switch(operation_to_be_done){
		case 65: xil_printf("Addition operation\n");    result=store_operands[0]+store_operands[1]; break;
		case 66: xil_printf("Subtraction operation\n"); result=store_operands[1]-store_operands[0]; break;
		case 67: xil_printf("Multiplication operation\n"); result=store_operands[0]*store_operands[1]; break;
		case 68: xil_printf("Factorial operation\n"); if(store_operands[0]<=store_operands[1]) result=factorial(store_operands[0]); else result=factorial(store_operands[1]); break;
		default:result = 0; break;
		}

		xil_printf("Arithmetic operation result = %d\n\n",result);


		vTaskPrioritySet( xTxTask, ( uxPriority + 1 ) );

	}
}

u32 factorial(int n1){

	u32 factorial_answer=1;
	for(int i=1; i<=n1; i++){
		factorial_answer *= i;

		if(factorial_answer>=4294967296){
			xil_printf("Range overflow for 32 bit number\n");
			return 0;
		}
	}
	xil_printf("factorial_answer =%d\n",factorial_answer);

	return factorial_answer;
}

