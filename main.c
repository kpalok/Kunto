/*
 *  ======== main.c ========
 */
/* XDCtools Header files */
#include <xdc/std.h>
#include <stdio.h>
#include <string.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>

/* TI-RTOS Header files */
#include <ti/drivers/I2C.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/i2c/I2CCC26XX.h>

#include <ti/mw/display/Display.h>

/* Board Header files */
#include "Board.h"
#include "Algorithm.h"
#include "Graphics.h"

/* JTKJ Header files */
#include "wireless/comm_lib.h"
#include "wireless/address.h"
#include "sensors/mpu9250.h"
#include "sensors/bmp280.h"

/* Task Stacks */
#define STACKSIZE 2048
Char mainTaskStack[STACKSIZE];
Char displayTaskStack[STACKSIZE];
Char commTaskStack[STACKSIZE];

/* MPU Global variables */
static PIN_Handle hMpuPin;
static PIN_State MpuPinState;
static PIN_Config MpuPinConfig[] = {
    Board_MPU_POWER  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
	 .pinSDA = Board_I2C0_SDA1,
	 .pinSCL = Board_I2C0_SCL1
};

Display_Handle hDisplay;

/*Global flag*/
bool firstSecondOfMeasurement = true;

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

Void DrawMovementState(uint8_t counter);
tImage* SelectStairsUpImg(uint8_t counter);
tImage* SelectStairsDownImg(uint8_t counter);
tImage* SelectLiftUpImg(uint8_t counter);
tImage* SelectLiftDownImg(uint8_t counter);


Void stateButtonFxn(PIN_Handle handle, PIN_Id pinId){

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
	I2C_Handle i2cMPU; // Interface for MPU9250
	I2C_Params i2cMPUParams;
	I2C_Handle i2c;	//Interface for BMP280
	I2C_Params i2cParams;

	float ax, ay, az, gx, gy, gz;
	double pres, temperature;
	float axSet[10], aySet[10], azSet[10];
	double presSet[10], prevPresSet[10], temperatureSet[10];
	int i = 0;

    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;

	I2C_Params_init(&i2cMPUParams);
	i2cMPUParams.bitRate = I2C_400kHz;
	i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;

	i2cMPU = I2C_open(Board_I2C, &i2cMPUParams); //MPU Open I2C
	if (i2cMPU == NULL) {
		System_abort("Error Initializing I2CMPU\n");
	}

	PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_ON); //MPU Power ON
	Task_sleep(100000 / Clock_tickPeriod);

	System_printf("MPU9250: Power ON\n");
	System_flush();

	System_printf("MPU9250: Setup and calibration...\n");
	System_flush();

	mpu9250_setup(&i2cMPU);  // MPU9250 Setup and calibration

	System_printf("MPU9250: Setup and calibration OK\n");
	System_flush();

	I2C_close(i2cMPU); //MPU9250 Close

	i2c = I2C_open(Board_I2C, &i2cParams); //BMP280 Open
	if (i2c == NULL) {
		System_abort("Error Initializing I2C\n");
	}

	bmp280_setup(&i2c); //BMP280 SETUP

	I2C_close(i2c); //BMP280 Close

	while (1) {
		i2c = I2C_open(Board_I2C, &i2cParams); //BMP280 Open I2C
		if (i2c == NULL) {
			System_abort("Error Initializing I2C\n");
		}

		bmp280_get_data(&i2c, &pres, &temperature); //Get pres and temp values from sensor

		I2C_close(i2c);
		
		prevPresSet[i] = pressureSet[i];
		pressureSet[i] = pres / 100; // convert pressure unit from pascal to hehtopascal

		temperatureSet[i] = temperature;

		i2cMPU = I2C_open(Board_I2C, &i2cMPUParams); //MPU9250 Open I2C
		if (i2cMPU == NULL) {
			System_abort("Error Initializing I2CMPU\n");
		}

		mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz); //Get accelerometer values from sensor

		I2C_close(i2cMPU);

		axSet[i] = ax;
		aySet[i] = ay;
		azSet[i] = az;
		i++;

		// Dont canculate state from firs seconds data, because pressure has no referense set to last second
		if (i == 10 & !firstSecondOfMeasurement){
			state = CalcState(axSet, aySet, azSet, pressureSet, prevPresSet);
			i = 0;
		}
		else if (i == 10 &){
			firstSecondOfMeasurement = false;
			i = 0;
		}

		Task_sleep(100000 / Clock_tickPeriod);
	}
}


Void DrawMovementState(uint8_t counter){

	if (hDisplay){
		Display_clear(hDisplay);

		tContext *pContext = DisplayExt_getGrlibContext(hDisplay);

		if (pContext){
			switch (state){
			case Idle:
				GrImageDraw(pContext, &idleImage, 0, 0);
				break;
			case StairsUp:
				GrImageDraw(pContext, SelectStairsUpImg(counter), 0, 0);
			break;
			case StairsDown:
				GrImageDraw(pContext, SelectStairsDownImg(counter), 0, 0);
				break;
			case LiftUp:
				GrImageDraw(pContext, SelectLiftUpImg(counter), 0, 0);
				break;
			case LiftDown:
				GrImageDraw(pContext, SelectLiftDownImg(counter), 0, 0);
				break;
			}

			GrFlush(pContext);
		}
	}
}

tImage* SelectStairsUpImg(uint8_t counter){

	switch(counter){
	case 1:
		return (tImage*)&stairsUpImage1;
	case 2:
		return (tImage*)&stairsUpImage2;
	case 3:
		return (tImage*)&stairsUpImage3;
	case 4:
		return (tImage*)&stairsUpImage4;
	case 5:
		return (tImage*)&stairsUpImage5;
	default:
		return (tImage*)&stairsUpImage3;
	}
}

tImage* SelectStairsDownImg(uint8_t counter){

	switch(counter){
	case 1:
		return (tImage*)&stairsDownImage1;
	case 2:
		return (tImage*)&stairsDownImage2;
	case 3:
		return (tImage*)&stairsDownImage3;
	case 4:
		return (tImage*)&stairsDownImage4;
	case 5:
		return (tImage*)&stairsDownImage5;
	default:
		return (tImage*)&stairsDownImage3;
	}
}

tImage* SelectLiftUpImg(uint8_t counter){

	switch(counter){
	case 1:
		return (tImage*)&liftUpImage1;
	case 2:
		return (tImage*)&liftUpImage2;
	case 3:
		return (tImage*)&liftUpImage3;
	case 4:
		return (tImage*)&liftUpImage4;
	case 5:
		return (tImage*)&liftUpImage5;
	default:
		return (tImage*)&liftUpImage3;
	}
}

tImage* SelectLiftDownImg(uint8_t counter){

	switch(counter){
	case 1:
		return (tImage*)&liftDownImage1;
	case 2:
		return (tImage*)&liftDownImage2;
	case 3:
		return (tImage*)&liftDownImage3;
	case 4:
		return (tImage*)&liftDownImage4;
	case 5:
		return (tImage*)&liftDownImage5;
	default:
		return (tImage*)&liftDownImage3;
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

    uint8_t counter = 1;

    while (1) {
    	counter++;

    	if ((counter % 4) == 0){
			DrawMovementState(counter / 2);
    	}

    	if (counter == 20){
    		counter = 1;
    	}
    	Task_sleep(50000 / Clock_tickPeriod);
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
    mainTaskParams.stack = &mainTaskStack;
    mainTaskParams.priority=3;

    hMainTask = Task_create((Task_FuncPtr)sensorFxn, &mainTaskParams, NULL);
    if (hMainTask == NULL) {
    	System_abort("Task create failed!");
    }

    /*Init Display Task */
    Task_Params_init(&displayTaskParams);
    displayTaskParams.stackSize = STACKSIZE;
    displayTaskParams.stack = &displayTaskStack;
    displayTaskParams.priority=2;

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

