/*
 * IMPLEMENTATION OF A SINGLE DIGIT CALCULATOR.
 * Inputs Operands from the keypad
 * Output of the arithmetic operation is displayed on the Seven Segment
 * Operations available : +, -, * and /, selected using the push buttons on the board, 0001 for +, 0010 for -, 0100 for * and 1000 for /
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
#include "math.h"

// Parameter definitions
#define BTNS_DEVICE_ID		XPAR_AXI_GPIO_BUTTONS_DEVICE_ID
#define SSD_DEVICE_ID		XPAR_AXI_GPIO_PMOD_SSD_DEVICE_ID

//Button Variable
XGpio BTNInst, SSDInst;

/* The Tx and Rx tasks as described at the top of this file. */
static void prvTxTask( void *pvParameters );
static void prvRxTask( void *pvParameters );

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

u32 SSD_decode(u8 key_value, u8 cathode){

	switch(key_value){
	case 0: if(cathode==0) return 0b00111111; else return 0b10111111;
	case 1: if(cathode==0) return 0b00000110; else return 0b10000110;
	case 2: if(cathode==0) return 0b01011011; else return 0b11011011;
	case 3: if(cathode==0) return 0b01001111; else return 0b11001111;
	case 4: if(cathode==0) return 0b01100110; else return 0b11100110;
	case 5: if(cathode==0) return 0b01101101; else return 0b11101101;
	case 6: if(cathode==0) return 0b01111101; else return 0b11111101;
	case 7: if(cathode==0) return 0b00000111; else return 0b10000111;
	case 8: if(cathode==0) return 0b01111111; else return 0b11111111;
	case 9: if(cathode==0) return 0b01101111; else return 0b11101111;
	default:if(cathode==0) return 0b00000000; else return 0b00000000;
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

  // Initialise Push Buttons
  status = XGpio_Initialize(&BTNInst, BTNS_DEVICE_ID);
  if(status != XST_SUCCESS){
	  xil_printf("GPIO Initialization for BUTTONS unsuccessful.\r\n");
	  return XST_FAILURE;
  }

  // Initialize SSD
  status = XGpio_Initialize(&SSDInst, SSD_DEVICE_ID);
  if(status != XST_SUCCESS){
    xil_printf("GPIO Initialization for SSD unsuccessful.\r\n");
    return XST_FAILURE;
  }

  // Set all buttons direction to inputs
  XGpio_SetDataDirection(&BTNInst, 1, 0xFF);
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
  xQueue = xQueueCreate( 2,				/* There is only one space in the queue. */
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
	   u8 key, last_key = 'x', store_key;
	   u32 key_stroke_on_SSD=0;

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
	      if(uxQueueMessagesWaiting( xQueue ) == 2){
	    	  xil_printf("Going to task 2!\n");
	   		  vTaskPrioritySet( NULL, ( uxPriority - 2 ) );
	      }

	      // Print key detect if a new key is pressed or if status has changed
	      if (status == KYPD_SINGLE_KEY
	            && (status != last_status || key != last_key)) {
	         xil_printf("Key Pressed: %c\r\n", (char) key);
	         last_key = key;

	         //Keys 'A', 'B', 'C', 'D' and 'F' will be ignored for this part of the lab 1. So, if user hits these keys, consider them invalid.
	         if((char)key == 'A' || (char)key == 'B' || (char)key == 'C' || (char)key == 'D' || (char)key == 'F'){
	        	 xil_printf(" XXX Not a valid key press! Keys A/B/C/D/F are to be ignored XXX\n");
	         }
	         //case when we consider input key strokes from '0' to '9' (only these are the valid key inputs for arithmetic operation
	         else if((char)key != 'E' ){
	        	 store_key = key;
//	        	 xil_printf("store_key = %c\n",(char)key);
	         }
	         //if user presses 'E' key, consider the last input key pressed as the operand
	         else if((char)key == 'E'){
	        	 xil_printf("Storing the operand %d to Queue...\n",(char) store_key);
		    	 key_stroke_on_SSD = SSD_decode((int)store_key-48, 1);
			     XGpio_DiscreteWrite(&SSDInst, 1, key_stroke_on_SSD);
		         //Length of queue=2, hence we only store the key pressed value in queue, when 'E' will be pressed. Once the queue is full with two values, using dynamic priority change
		         //we increased the priority of receive where the queue will be read and arithmetic operations will be done.
		         xQueueSend( xQueue,			/* The queue being written to. */
		         			 &store_key, 		/* The address of the data being sent. */
		         			 0UL );				/* The block time. */
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
		u8 read_queue_value;
		u32 store_operands[2];
		u32 detected_value=0;
		int result=0; //"int" because we may get negative results in subtraction.

		unsigned int btn_value;

		while(uxQueueMessagesWaiting(xQueue)!=0){

//			xil_printf("queue items = %llU\n",uxQueueMessagesWaiting(xQueue));

			/* Block to wait for data arriving on the queue. */
			xQueueReceive( 	xQueue,				/* The queue being read. */
							&read_queue_value,	/* Data is read into this address. */
							portMAX_DELAY );	/* Wait without a timeout for data. */

//			xil_printf("Received value :: %c\n",(char)led_value);

			//keypad values are always ascii value so to convert them into "int" we use the following logic
			if(read_queue_value>=48 && read_queue_value<=57)
				detected_value = (int)read_queue_value - 48;
			else
				detected_value = (int)read_queue_value - 65 + 10;

			//in total two values will be read from the queue so, store them in arrays as two operands.
			if(uxQueueMessagesWaiting(xQueue)==1)
				store_operands[0]=detected_value;
			else if(uxQueueMessagesWaiting(xQueue)==0)
				store_operands[1]=detected_value;

		}

		xil_printf("store_operands[0]=%d, store_operands[1]=%d\n", store_operands[0],store_operands[1]);

		btn_value = XGpio_DiscreteRead(&BTNInst,1);
		xil_printf("btn_value = %d\n\n",btn_value);

		//keep the button pressed for your choice of the arithmetic operation
		switch(btn_value){
		case 1: result=store_operands[0]+store_operands[1]; break;
		case 2: result=store_operands[1]-store_operands[0]; break;
		case 4: result=store_operands[0]*store_operands[1]; break;
		case 8: result=store_operands[1]/store_operands[0]; break;
		default:result = 0; break;
		}

		xil_printf("Arithmetic operation result = %d\n\n",result);

		//the following logic is to extract the digits from the result. For example, 9x9=81 so we will first display 1 on right SSD and then 8 on the left SSD!
		//please note that our operands are between 0 to 9 only. So in any case of +/-/*/div, we will never exceed a results more than two digits!

		u8 cathode_signal = 0; // cathode_signal is for selecting right SSD and left SSD one after the other.

		if(result<0)
			xil_printf("Result is less than zero!!!\n");
		else if(result==0){
			XGpio_DiscreteWrite(&SSDInst, 1, 0b00111111);
		    vTaskDelay(pdMS_TO_TICKS(1000)); //means 1000ms
		}

		vTaskDelay(pdMS_TO_TICKS(1500)); //this delay is merely to introduce the visual difference between the input and the output result!

		while(result>0){
			u32 ssd_value;
			u8 mod = result % 10;
//			xil_printf("%d\n", mod);
//			xil_printf("cathode_signal = %d\n",cathode_signal);
			ssd_value = SSD_decode(mod, cathode_signal); //display right segment first and then after a short delay display the left SSD. cathode_signal will be either 0 or 1.
		    XGpio_DiscreteWrite(&SSDInst, 1, ssd_value );
		    vTaskDelay(pdMS_TO_TICKS(1000)); //means 1000ms
		    result = result/10;

		    if(cathode_signal==1) cathode_signal = 0;
		    else cathode_signal = cathode_signal + 1;

		}

		//clear both the segments now after the result is displayed.

		XGpio_DiscreteWrite(&SSDInst, 1, 0b00000000);
		XGpio_DiscreteWrite(&SSDInst, 1, 0b10000000);

		//we are now done doing the calculation so again go back to the task 1 (TxTask) to get the new inputs!
		vTaskPrioritySet( xTxTask, ( uxPriority + 1 ) );

	}
}

