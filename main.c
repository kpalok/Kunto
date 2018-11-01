/*
 *  ======== main.c ========
 */
/* XDCtools Header files */
#include <xdc/std.h>
#include <stdio.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>

/* TI-RTOS Header files */
#include <ti/drivers/I2C.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>

#include <ti/mw/display/Display.h>

/* Board Header files */
#include "Board.h"

#include "Graphics.h"

/* JTKJ Header files */
#include "wireless/comm_lib.h"
#include "wireless/address.h"

#include "sensors/bmp280.h"

/* Task Stacks */
#define STACKSIZE 2048
Char mainTaskStack[STACKSIZE];
Char commTaskStack[STACKSIZE];

/* JTKJ: Display */
Display_Handle hDisplay;

/*Enum for tracking movement state */
enum MovementState{
	Idle = 0,
	StairsUp = 1,
	StairsDown = 2,
	LiftUp = 3,
	LiftDown = 4
};

enum MovementState state = Idle;

/* JTKJ: Pin Button1 configured as power button */
static PIN_Handle hPowerButton;
static PIN_State sPowerButton;
PIN_Config cPowerButton[] = {
    Board_BUTTON1 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    PIN_TERMINATE
};
PIN_Config cPowerWake[] = {
    Board_BUTTON1 | PIN_INPUT_EN | PIN_PULLUP | PINCC26XX_WAKEUP_NEGEDGE,
    PIN_TERMINATE
};

/* JTKJ: Pin Button0 configured as input */
static PIN_Handle hButton0;
static PIN_State sButton0;
PIN_Config cButton0[] = {
    Board_BUTTON0 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    PIN_TERMINATE
};

Void DrawMovementState(void);

Void stateButtonFxn(PIN_Handle handle, PIN_Id pinId){
	if (state < LiftDown){
		state++;
	}
	else{
		state = Idle;
	}

	DrawMovementState();
}


/* JTKJ: Handle for power button */
Void powerButtonFxn(PIN_Handle handle, PIN_Id pinId) {

    Display_clear(hDisplay);
    Display_close(hDisplay);
    Task_sleep(100000 / Clock_tickPeriod);

	PIN_close(hPowerButton);

    PINCC26XX_setWakeup(cPowerWake);
	Power_shutdown(NULL,0);
}

/* JTKJ: Communication Task */
Void commTask(UArg arg0, UArg arg1) {

    // Radio to receive mode
	int32_t result = StartReceive6LoWPAN();
	if(result != true) {
		System_abort("Wireless receive mode failed");
	}
	/*
	char send_str[8];
	sprintf(send_str, "Nerf Olm");

	Send6LoWPAN(IEEE80154_SERVER_ADDR, &send_str, strlen(send_str));
	*/
	StartReceive6LoWPAN();


    while (1) {

        // DO __NOT__ PUT YOUR SEND MESSAGE FUNCTION CALL HERE!! 

    	if(GetRXFlag()){

    	}
        
    }
}


Int JoonanAlgoritmi(){


	return Idle;
}

Void DrawMovementState(void){

	if (hDisplay){
		Display_clear(hDisplay);

		tContext *pContext = DisplayExt_getGrlibContext(hDisplay);

		if (pContext){
			switch (state){
			case Idle:
				GrImageDraw(pContext, &idleImage, 0, 0);
				break;
			case StairsUp:
				GrImageDraw(pContext, &stairsUpImage, 0, 0);
			break;
			case StairsDown:
				GrImageDraw(pContext, &stairsDownImage, 0, 0);
				break;
			case LiftUp:
				GrImageDraw(pContext, &liftUpImage, 0, 0);
				break;
			case LiftDown:
				GrImageDraw(pContext, &liftDownImage, 0, 0);
				break;
			}

			GrFlush(pContext);
		}
	}
}

Void mainTask(UArg arg0, UArg arg1) {

    I2C_Handle      i2c;
    I2C_Params      i2cParams;

    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;
    i2c = I2C_open(Board_I2C0, &i2cParams);
    if (i2c == NULL) {
        System_abort("Error Initializing I2C\n");
    }

    Display_Params displayParams;
	displayParams.lineClearMode = DISPLAY_CLEAR_BOTH;
    Display_Params_init(&displayParams);

    hDisplay = Display_open(Display_Type_LCD, &displayParams);
    if (hDisplay == NULL) {
        System_abort("Error initializing Display\n");
    }

    DrawMovementState();

    while (1) {

    	// JTKJ: MAYBE READ BMP280 SENSOR DATA HERE?

    	// JTKJ: Do not remove sleep-call from here!
    	Task_sleep(1000000 / Clock_tickPeriod);
    }
}

Int main(void) {

	Task_Handle hMainTask;
	Task_Params mainTaskParams;
	Task_Handle hCommTask;
	Task_Params commTaskParams;

    // Initialize board
    Board_initGeneral();
    Board_initI2C();

	hPowerButton = PIN_open(&sPowerButton, cPowerButton);
	if(!hPowerButton) {
		System_abort("Error initializing power button shut pins\n");
	}
	if (PIN_registerIntCb(hPowerButton, &powerButtonFxn) != 0) {
		System_abort("Error registering power button callback function");
	}

    hButton0 = PIN_open(&sButton0, cButton0);
    if (!hButton0){
    	System_abort("Error initializing button0 shut pins \n");
    }
	if (PIN_registerIntCb(hButton0, &stateButtonFxn) != 0) {
		System_abort("Error registering led button callback function");
	}

    /*Init Main Task */
    Task_Params_init(&mainTaskParams);
    mainTaskParams.stackSize = STACKSIZE;
    mainTaskParams.stack = &mainTaskStack;
    mainTaskParams.priority=2;

    hMainTask = Task_create(mainTask, &mainTaskParams, NULL);
    if (hMainTask == NULL) {
    	System_abort("Task create failed!");
    }

    Init6LoWPAN();

    Task_Params_init(&commTaskParams);
    commTaskParams.stackSize = STACKSIZE;
    commTaskParams.stack = &commTaskStack;
    commTaskParams.priority=1;
    
    hCommTask = Task_create(commTask, &commTaskParams, NULL);
    if (hCommTask == NULL) {
    	System_abort("Task create failed!");
    }

    /* Start BIOS */
    BIOS_start();

    return (0);
}

