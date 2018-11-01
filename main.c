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
#include "sensors/mpu9250.h"

/* Task Stacks */
#define STACKSIZE 2048
Char mainTaskStack[STACKSIZE];
Char commTaskStack[STACKSIZE];

/* MPU Global variables */
static PIN_Handle hMpuPin;
static PIN_State MpuPinState;
static PIN_Config MpuPinConfig[] = {
    Board_MPU_POWER  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1
};

/* JTKJ: Display */
Display_Handle hDisplay;

/*Enum for tracking movement state */
typedef enum MovementState{
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


void sensorFxn(UArg arg0, UArg arg1) {
	// INTERFACE FOR MPU9250 SENSOR
	I2C_Handle i2cMPU;
	I2C_Params i2cMPUParams;

	float ax, ay, az, gx, gy, gz;

	I2C_Params_init(&i2cMPUParams);
	i2cMPUParams.bitRate = I2C_400kHz;
	i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;

	//MPU Open I2C
	i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
	if (i2cMPU == NULL) {
		System_abort("Error Initializing I2CMPU\n");
	}


	//MPU Power ON
	PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_ON);
	Task_sleep(100000 / Clock_tickPeriod);
	System_printf("MPU9250: Power ON\n");
	System_flush();

	// MPU9250 Setup and calibration
	System_printf("MPU9250: Setup and calibration...\n");
	System_flush();

	mpu9250_setup(&i2cMPU);

	System_printf("MPU9250: Setup and calibration OK\n");
	System_flush();

	I2C_close(i2cMPU);

	while (1) {
		//MPU Open I2C
		i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
		if (i2cMPU == NULL) {
			System_abort("Error Initializing I2CMPU\n");
		}

		//Accelerometer values: ax,ay,az
		mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);

		I2C_close(i2cMPU);

		printf("x: %f, y: %f, z: %f\n", ax, ay, az);

		Task_sleep(1000 / Clock_tickPeriod);
	}
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

Void displayTask(UArg arg0, UArg arg1) {

    Display_Params displayParams;
	displayParams.lineClearMode = DISPLAY_CLEAR_BOTH;
    Display_Params_init(&displayParams);

    hDisplay = Display_open(Display_Type_LCD, &displayParams);
    if (hDisplay == NULL) {
        System_abort("Error initializing Display\n");
    }


    while (1) {

		DrawMovementState();

    	Task_sleep(100000 / Clock_tickPeriod);
    }
}

Int main(void) {

	Task_Handle hMainTask;
	Task_Params mainTaskParams;
	Task_Handle hDisplayTask;
	Task_Params displayTaskParams;
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

    hMpuPin = PIN_open(&MpuPinState, MpuPinConfig);
    if (hMpuPin == NULL) {
    	System_abort("Pin open failed!");
    }

    Task_Params_init(&mainTaskParams);
    mainTaskParams.stackSize = STACKSIZE;
    mainTaskParams.stack = &taskStack;
    mainTaskParams.priority=2;

    hMainTask = Task_create((Task_FuncPtr)sensorFxn, &mainTaskParams, NULL);
    if (hMainTask == NULL) {
    	System_abort("Task create failed!");
    }

    /*Init Display Task */
    Task_Params_init(&displayTaskParams);
    displayTaskParams.stackSize = STACKSIZE;
    displayTaskParams.stack = &displayTaskStack;
    displayTaskParams.priority=3;

    hDisplayTask = Task_create(displayTask, &displayTaskParams, NULL);
    if (hDisplayTask == NULL) {
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

