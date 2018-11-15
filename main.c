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
Char sensorTaskStack[STACKSIZE];
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

/*State variables*/
typedef enum windowState {
	Measurement = 0,
	MessageWaiting = 1,
	Communication = 2
} WindowState;

bool firstSecondOfMeasurement = true;
bool walkedStairs = false;
bool tookLift = false;
bool windowChanged = false;
WindowState windowState = Measurement;
MovementState movementState = Idle;

double globalPres, globalTemperature;

uint16_t previousSenderAddress;
char previousReceivedMessage[16], previousSentMessage[16];


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

Void DrawNotificationIcon();
Void DrawCommunicationLog();
Void DrawMovementState(uint8_t counter);
void DrawBmpSensorData();

tImage* SelectStairsUpImg(uint8_t counter);
tImage* SelectStairsDownImg(uint8_t counter);
tImage* SelectLiftUpImg(uint8_t counter);
tImage* SelectLiftDownImg(uint8_t counter);


Void stateButtonFxn(PIN_Handle handle, PIN_Id pinId) {
	// Change window state between measurement and communication
	if (windowState != Communication) {
		windowState = Communication;
	}
	else {
		// If there was message waiting it's now checked
		firstSecondOfMeasurement = true;
		windowState = Measurement;
	}

	windowChanged = true;
}

/* JTKJ: Handle for power button */
Void powerButtonFxn(PIN_Handle handle, PIN_Id pinId) {

	Display_clear(hDisplay);
	Display_close(hDisplay);
	Task_sleep(100000 / Clock_tickPeriod);

	PIN_close(hPowerButton);

	PINCC26XX_setWakeup(cPowerWake);
	Power_shutdown(NULL, 0);
}

/*Communication Task */
Void commTask(UArg arg0, UArg arg1) {

	// Radio to receive mode
	int32_t result = StartReceive6LoWPAN();
	if (result != true) {
		System_abort("Wireless receive mode failed");
	}

	StartReceive6LoWPAN();

	while (1) {

		if (GetRXFlag()) {

			memset(previousReceivedMessage, 0, 16);

			Receive6LoWPAN(&previousSenderAddress, previousReceivedMessage, 16);

			// if message is received, change state so user is notified
			// if window isn't in communication state
			if (windowState == Measurement){
				windowState = MessageWaiting;
			}
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
	float axSet[20], aySet[20], azSet[20];
	double presSet[20], prevPresSet[20];
	int i = 0;

	MovementState previousState = Idle;

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

		if (windowState != Communication){

			i2c = I2C_open(Board_I2C, &i2cParams); //BMP280 Open I2C
			if (i2c == NULL) {
				System_abort("Error Initializing I2C\n");
			}

			bmp280_get_data(&i2c, &pres, &temperature); //Get pres and temp values from sensor

			I2C_close(i2c);

			prevPresSet[i] = presSet[i];
			presSet[i] = pres / 100; // convert pressure unit from pascal to hehtopascal

			globalPres = pres / 100;
			globalTemperature = ((temperature - 32) * 5) / 9; // convert temperature unit from fahrenheit to celsius

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
			if (i == 20 & !firstSecondOfMeasurement){
				movementState = CalcState(axSet, aySet, azSet, presSet, prevPresSet);
				i = 0;
			}
			else if (i == 20){
				firstSecondOfMeasurement = false;
				i = 0;
			}

			if ((previousState == StairsUp | previousState == StairsDown) &
					movementState == Idle){

				walkedStairs = true;
			}
			else if ((previousState == LiftUp | previousState == LiftDown) &
					movementState == Idle){

				tookLift = true;
			}

			previousState = movementState;
		}

		Task_sleep(100000 / Clock_tickPeriod);
	}
}


Void DrawBmpSensorData(){

	if (hDisplay){
		tContext *pContext = DisplayExt_getGrlibContext(hDisplay);

		if (pContext){

			GrStringDraw(pContext, "Temp:", -1, 48, 0, true);

			char temperatureStr[8];
			sprintf(temperatureStr, "%.2f", globalTemperature);

			GrStringDraw(pContext, temperatureStr, -1, 48, 10, true);

			GrStringDraw(pContext, "Pres:", -1, 48, 30, true);

			char presStr[8];
			sprintf(presStr, "%.2f", globalPres);

			GrStringDraw(pContext, presStr, -1, 48, 40, true);
		}

	}
}



Void DrawCommunicationLog() {

	if (hDisplay) {

		Display_print0(hDisplay, 0, 0, "Previous");
		Display_print0(hDisplay, 1, 0, "received");
		Display_print0(hDisplay, 2, 0, "message:");
		if (previousSenderAddress > 0) {
			Display_print1(hDisplay, 3, 0, "From: %x", previousSenderAddress);
			Display_print0(hDisplay, 5, 0, previousReceivedMessage);
		}
		else {
			Display_print0(hDisplay, 4, 0, "No message");
		}

		Display_print0(hDisplay, 7, 0, "Previous");
		Display_print0(hDisplay, 8, 0, "sent message:");
		if (previousSenderAddress > 0) {
			Display_print0(hDisplay, 10, 0, previousSentMessage);
		}
		else {
			Display_print0(hDisplay, 10, 0, "No message");
		}
	}
}

Void DrawNotificationIcon() {

	if (hDisplay) {
		tContext *pContext = DisplayExt_getGrlibContext(hDisplay);

		if (pContext) {
			GrImageDraw(pContext, &envelopeImage, 47, 47);
		}
	}
}

Void DrawMovementState(uint8_t counter) {

	if (hDisplay) {
		tContext *pContext = DisplayExt_getGrlibContext(hDisplay);

		if (pContext) {
			switch (movementState) {
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

tImage* SelectStairsUpImg(uint8_t counter) {

	switch (counter) {
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

tImage* SelectStairsDownImg(uint8_t counter) {

	switch (counter) {
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

tImage* SelectLiftUpImg(uint8_t counter) {

	switch (counter) {
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

tImage* SelectLiftDownImg(uint8_t counter) {

	switch (counter) {
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

	bool popUpShowing = false;

	hDisplay = Display_open(Display_Type_LCD, &displayParams);

	if (hDisplay == NULL) {
		System_abort("Error initializing Display\n");
	}

	uint8_t frameCounter = 1;
	uint8_t secondCounter = 0;

	while (1) {
		// Display needs to be cleared only when window is changed
		if (windowChanged){
			Display_clear(hDisplay);
			windowChanged = false;
		}

		if (windowState != Communication) {

			DrawBmpSensorData();
			DrawMovementState(frameCounter);

			// reset animation by reseting counter every second
			if (frameCounter == 5) {
				secondCounter++;
				frameCounter = 0;
			}

			// reset second counter every two seconds, and clear popup if it's shown
			if (secondCounter == 2 & popUpShowing){
				Display_clearLines(hDisplay, 7, 15);
				popUpShowing = false;
				secondCounter = 0;
			}
			else if (secondCounter == 2){
				secondCounter = 0;
			}

			// see if messages are waiting and draw notifier
			if (windowState == MessageWaiting) {
				DrawNotificationIcon();
			}
			// draw and send encouraging message if stairs were used
			if (walkedStairs) {
				Display_print0(hDisplay, 9, 1, "Good");
				Display_print0(hDisplay, 10, 1, "job!");
				// raise popup shown flag
				popUpShowing = true;

				char sendStr[9];
				sprintf(sendStr, "In stairs");

				Send6LoWPAN(0xFFFF, sendStr, strlen(sendStr));
				StartReceive6LoWPAN();

				strcpy(previousSentMessage, sendStr);
				secondCounter = 0;
				walkedStairs = false;
			}
			else if (tookLift){

				Display_print0(hDisplay, 9, 1, "Stop");
				Display_print0(hDisplay, 10, 1, "slacking!");
				// raise popup shown flag
				popUpShowing = true;

				char sendStr[7];
				sprintf(sendStr, "In lift");

				Send6LoWPAN(0xFFFF, sendStr, strlen(sendStr));
				StartReceive6LoWPAN();

				strcpy(previousSentMessage, sendStr);
				secondCounter = 0;
				tookLift = false;
			}

			frameCounter++;
		}
		else {
			DrawCommunicationLog();
		}

		Task_sleep(200000 / Clock_tickPeriod);
	}
}


Int main(void) {

	Task_Handle hSensorTask;
	Task_Params sensorTaskParams;
	Task_Handle hDisplayTask;
	Task_Params displayTaskParams;
	Task_Handle hCommTask;
	Task_Params commTaskParams;

	// Initialize board
	Board_initGeneral();
	Board_initI2C();

	hPowerButton = PIN_open(&sPowerButton, cPowerButton);
	if (!hPowerButton) {
		System_abort("Error initializing power button shut pins\n");
	}
	if (PIN_registerIntCb(hPowerButton, &powerButtonFxn) != 0) {
		System_abort("Error registering power button callback function");
	}

	hButton0 = PIN_open(&sButton0, cButton0);
	if (!hButton0) {
		System_abort("Error initializing button0 shut pins \n");
	}
	if (PIN_registerIntCb(hButton0, &stateButtonFxn) != 0) {
		System_abort("Error registering led button callback function");
	}

	hMpuPin = PIN_open(&MpuPinState, MpuPinConfig);
	if (hMpuPin == NULL) {
		System_abort("Pin open failed!");
	}

	Task_Params_init(&sensorTaskParams);
	sensorTaskParams.stackSize = STACKSIZE;
	sensorTaskParams.stack = &sensorTaskStack;
	sensorTaskParams.priority = 3;

	hSensorTask = Task_create((Task_FuncPtr)sensorFxn, &sensorTaskParams, NULL);
	if (hSensorTask == NULL) {
		System_abort("Task create failed!");
	}

	/*Init Display Task */
	Task_Params_init(&displayTaskParams);
	displayTaskParams.stackSize = STACKSIZE;
	displayTaskParams.stack = &displayTaskStack;
	displayTaskParams.priority = 2;

	hDisplayTask = Task_create(displayTask, &displayTaskParams, NULL);
	if (hDisplayTask == NULL) {
		System_abort("Task create failed!");
	}

	Init6LoWPAN();

	Task_Params_init(&commTaskParams);
	commTaskParams.stackSize = STACKSIZE;
	commTaskParams.stack = &commTaskStack;
	commTaskParams.priority = 1;

	hCommTask = Task_create(commTask, &commTaskParams, NULL);
	if (hCommTask == NULL) {
		System_abort("Task create failed!");
	}

	/* Start BIOS */
	BIOS_start();

	return (0);
}

