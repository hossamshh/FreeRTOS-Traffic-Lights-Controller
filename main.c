#include <stdint.h>
#include <stdbool.h>

#include "tm4c123gh6pm.h"

#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"

#include "FreeRTOS.h"
#include "task.h"

#define GPIO_PF0_M1PWM4         0x00050005
#define GPIO_PA6_M1PWM2         0x00001805
#define GPIO_PE4_M0PWM4         0x00041004

#define BLINK_FREQUENCY 1

#define tgn				5000
#define tge				2500
#define tcross		10000
#define tsafety		30000

#define rRate			500
#define tgnr			tgn/rRate
#define tger			tge/rRate
#define tcrossr		tcross/rRate	
#define tsafetyr	tsafety/rRate
#define blinkTime	1/BLINK_FREQUENCY*500/rRate

xTaskHandle northRedTaskHandle, northGreenTaskHandle, stateHandle, pdHandle, trainHandle;

enum STATE {NGREEN_ERED, NRED_EGREEN, WAIT_TGN, WAIT_TGE, PC_NORTH, PC_EAST, TC_RIGHT_NORTH,
						TC_LEFT_NORTH, TC_RIGHT_EAST, TC_LEFT_EAST};

enum STATE newState = NGREEN_ERED;

bool pdPressed = false, rightPressed = false, leftPressed = false, trainf = false, ngreenf = true, egreenf = false, predf = false;


void openGate(){
	//angel 90
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 18750);
	PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, true);
	SysCtlDelay(2999999);
	PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, false);
}

void closeGate(){
	//angel 0
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 9500);
	PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, true);
	SysCtlDelay(2999099);
	PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, false);
}

void siren(){
	if((GPIO_PORTE_DATA_R & 0x08) == 0x08){
		GPIO_PORTE_DATA_R &= ~0x08;
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_4, 7999);
		}
	else{
		GPIO_PORTE_DATA_R |= 0x08;
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_4, 5999);
	}
}

void vApplicationIdleHook(){
	if((GPIO_PORTE_DATA_R & 0x01) == 0x01)
		pdPressed = true;
	else if((GPIO_PORTE_DATA_R & 0x02) == 0x02)
		rightPressed = true;
	else if ((GPIO_PORTE_DATA_R & 0x04) == 0x04)
		leftPressed = true;
}

void stateHandler(){
	static int counter = 0;
	static int trainCounter = 0;
	
	while(1){

		switch(newState){
			case NGREEN_ERED:
				counter++;
				vTaskPrioritySet(trainHandle, 5);
				vTaskPrioritySet(pdHandle, 4);
				vTaskPrioritySet(northRedTaskHandle, 3);
				vTaskPrioritySet(northGreenTaskHandle, 2);
				if(leftPressed){
					trainf = true;
					newState = TC_LEFT_NORTH;
				}
				else if(rightPressed){
					trainf = true;
					newState = TC_RIGHT_NORTH;
				}
				else if(pdPressed){
					
					newState = WAIT_TGN;
				}
				else if(counter >= tgnr){
					counter = 0;
					egreenf = true;
					newState = NRED_EGREEN;
				}
				break;
				
			case NRED_EGREEN:
				counter++;
				vTaskPrioritySet(trainHandle, 5);
				vTaskPrioritySet(pdHandle, 4);
				vTaskPrioritySet(northGreenTaskHandle, 3);			
				vTaskPrioritySet(northRedTaskHandle, 2);			
				if(leftPressed){
					trainf = true;
					newState = TC_LEFT_EAST;
				}
				else if(rightPressed){
					trainf = true;
					newState = TC_RIGHT_EAST;
				}
				else if(pdPressed){
					
					newState = WAIT_TGE;
				}
				else if(counter >= tger){
					counter = 0;
					pdPressed = false;
					ngreenf = true;
					newState = NGREEN_ERED;
				}				
				break;
				
			case WAIT_TGE:
				counter++;
				if(leftPressed){
					trainf = true;
					newState = TC_LEFT_EAST;
				}
				else if(rightPressed){
					trainf = true;
					newState = TC_RIGHT_EAST;
				}
				else if(counter >= tger){
					counter = 0;
					pdPressed = false;
					predf = true;
					newState = PC_EAST;
				}
				break;
				
			case WAIT_TGN:
				counter++;
				if(leftPressed){
					trainf = true;
					newState = TC_LEFT_NORTH;
				}
				else if(rightPressed){
					trainf = true;
					newState = TC_RIGHT_NORTH;
				}
				if(counter >= tgnr){
					counter = 0;
					pdPressed = false;
					predf = true;
					newState = PC_NORTH;
				}
				break;
				
			case PC_NORTH:
				counter++;
				vTaskPrioritySet(trainHandle, 5);
				vTaskPrioritySet(northGreenTaskHandle, 4);			
				vTaskPrioritySet(northRedTaskHandle, 3);			
				vTaskPrioritySet(pdHandle, 2);
				if(leftPressed){
					trainf = true;
					counter = 0;
					newState = TC_LEFT_EAST;
				}
				else if(rightPressed){
					trainf = true;
					counter = 0;
					newState = TC_RIGHT_EAST;
				}
				else if(counter >= tcrossr){
					counter = 0;
					egreenf = true;
					newState = NRED_EGREEN;
				}
				break;
				
			case PC_EAST:
				counter++;
				vTaskPrioritySet(trainHandle, 5);
				vTaskPrioritySet(northGreenTaskHandle, 4);			
				vTaskPrioritySet(northRedTaskHandle, 3);			
				vTaskPrioritySet(pdHandle, 2);
				if(leftPressed){
					trainf = true;
					counter = 0;
					newState = TC_LEFT_NORTH;
				}
				else if(rightPressed){
					trainf = true;
					counter = 0;
					newState = TC_RIGHT_NORTH;
				}
				else if(counter >= tcrossr){
					counter = 0;
					ngreenf = true;
					newState = NGREEN_ERED;
				}				
				break;
				
			case TC_RIGHT_NORTH:
				trainCounter++;
				rightPressed = false;
				siren();
				vTaskPrioritySet(pdHandle, 5);
				vTaskPrioritySet(northGreenTaskHandle, 4);			
				vTaskPrioritySet(northRedTaskHandle, 3);			
				vTaskPrioritySet(trainHandle, 2);
				if(trainCounter >= tsafetyr && leftPressed){
					trainCounter = 0;
					leftPressed = false;
					ngreenf = true;
					PWMOutputState(PWM1_BASE, PWM_OUT_4_BIT, false);
					openGate();
					GPIO_PORTE_DATA_R &= ~0x08;
					newState = NGREEN_ERED;
				}	
				break;
				
			case TC_LEFT_NORTH:
				trainCounter++;
				leftPressed = false;
				siren();
				vTaskPrioritySet(pdHandle, 5);
				vTaskPrioritySet(northGreenTaskHandle, 4);			
				vTaskPrioritySet(northRedTaskHandle, 3);			
				vTaskPrioritySet(trainHandle, 2);
				if(trainCounter >= tsafetyr && rightPressed){
					trainCounter = 0;
					rightPressed = false;
					ngreenf = true;
					PWMOutputState(PWM1_BASE, PWM_OUT_4_BIT, false);
					openGate();
					GPIO_PORTE_DATA_R &= ~0x08;
					newState = NGREEN_ERED;
				}	
				break;
				
			case TC_RIGHT_EAST:
				trainCounter++;
				rightPressed = false;
				siren();
				vTaskPrioritySet(pdHandle, 5);
				vTaskPrioritySet(northGreenTaskHandle, 4);			
				vTaskPrioritySet(northRedTaskHandle, 3);			
				vTaskPrioritySet(trainHandle, 2);
				if(trainCounter >= tsafetyr && leftPressed){
					trainCounter = 0;
					leftPressed = false;
					egreenf = true;
					PWMOutputState(PWM1_BASE, PWM_OUT_4_BIT, false);
					openGate();
					GPIO_PORTE_DATA_R &= ~0x08;
					newState = NRED_EGREEN;
				}	
				break;
			
			case TC_LEFT_EAST:
				trainCounter++;
				leftPressed = false;
				siren();
				vTaskPrioritySet(pdHandle, 5);
				vTaskPrioritySet(northGreenTaskHandle, 4);			
				vTaskPrioritySet(northRedTaskHandle, 3);			
				vTaskPrioritySet(trainHandle, 2);
				if(trainCounter >= tsafetyr && rightPressed){
					trainCounter = 0;
					rightPressed = false;
					egreenf = true;
					PWMOutputState(PWM1_BASE, PWM_OUT_4_BIT, false);
					openGate();
					GPIO_PORTE_DATA_R &= ~0x08;
					newState = NRED_EGREEN;
				}	
				break;
		}
		
		vTaskDelay(rRate);
	}
}

void northRedLight(void *pvParameters){
	while(1){
		if(egreenf){
			GPIO_PORTB_DATA_R = 0x96;
			egreenf = false;
		}
		vTaskDelay(rRate);
	}
}

void northGreenLight(void *pvParameters){
	while(1){
		if(ngreenf){
			GPIO_PORTB_DATA_R = 0x69;
			ngreenf = false;
		}
		vTaskDelay(rRate);
	}
}

void pedestrainCrossing(void *pvParameters){
	while(1){
		if(predf){
			GPIO_PORTB_DATA_R = 0xf0;
			predf = false;
		}
		vTaskDelay(rRate);
	}
}

void trainCrossing(void *pvParameters){
	while(1){
		if(trainf){
			GPIO_PORTB_DATA_R = 0xf0;
			siren();
			closeGate();
			PWMOutputState(PWM1_BASE, PWM_OUT_4_BIT, true);
			trainf = false;
		}
		vTaskDelay(rRate);
	}
}

void portBOutputSetup(){
	SYSCTL_RCGCGPIO_R |= 0x02;
	uint32_t dummy = SYSCTL_RCGCGPIO_R;
	
  GPIO_PORTB_DIR_R |= 0xff;  
  GPIO_PORTB_DEN_R |= 0xff;
}

void portESetup(){
	SYSCTL_RCGCGPIO_R |= 0x10;
	uint32_t dummy = SYSCTL_RCGCGPIO_R;
	
  GPIO_PORTE_DEN_R |= 0x0f; 
	GPIO_PORTE_DIR_R |= 0x08;
  GPIO_PORTE_DIR_R &= ~0x07;
	GPIO_PORTE_PDR_R |= 0x07;
}


void pwmSetup(){
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
	SysCtlPWMClockSet(SYSCTL_PWMDIV_4);
	
	GPIO_PORTF_LOCK_R = 0x4c4f434b;
	GPIO_PORTF_CR_R = 0x01f;
	
	GPIOPinConfigure(GPIO_PF0_M1PWM4);
	
	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_0);
	
	PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
	
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_2, 15999);
	
	PWMGenEnable(PWM1_BASE, PWM_GEN_2);

	
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
	
	GPIOPinConfigure(GPIO_PE4_M0PWM4);
	
	GPIOPinTypePWM(GPIO_PORTE_BASE, GPIO_PIN_4);
	
	PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
	
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, 250000);
	
	PWMGenEnable(PWM0_BASE, PWM_GEN_2);

}


int main(){
	portBOutputSetup();
	portESetup();
	pwmSetup();
	
	xTaskCreate(trainCrossing, NULL, configMINIMAL_STACK_SIZE, NULL, 5, &trainHandle);
	xTaskCreate(pedestrainCrossing, NULL, configMINIMAL_STACK_SIZE, NULL, 4, &pdHandle);
	xTaskCreate(northRedLight, NULL, configMINIMAL_STACK_SIZE, NULL, 3, &northRedTaskHandle);
	xTaskCreate(northGreenLight, NULL, configMINIMAL_STACK_SIZE, NULL, 2, &northGreenTaskHandle);
	xTaskCreate(stateHandler, NULL, 128, NULL, 1, &stateHandle);
	
	vTaskStartScheduler();
	
	return 0;
}