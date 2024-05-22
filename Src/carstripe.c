/*
 * carstripe.c
 *
 *  Created on: Apr 30, 2024
 *      Author: bartosz
 *
 *      A run effecf
 *      B stop effect
 *      C change color
 *      D change effect
 */

#include "carstripe.h"

//private variables
ws2812_Stripe_TypeDef stripe1;

wsfxEffect_TypeDef fxKnight;
wsfxEffect_TypeDef fxGlow;
wsfxEffect_TypeDef fxPulse;
wsfxEffect_TypeDef fxBlinker;
wsfxEffect_TypeDef fxStarting;
wsfxEffect_TypeDef fxWarp;
wsfxEffect_TypeDef fxHueChange;

radio433_receiverTypeDef radio;
radio433_transmitterTypeDef remote;
//radio433_transmitterTypeDef transmitter;

uint32_t carStripeButtonData;
int8_t carStripeEffectNumber = 0;
uint16_t carStripeColor;
uint8_t carStripeStartFlag = 0;
uint8_t carStripeStopFlag = 0;
uint8_t carStripeCommand = 0;
volatile uint16_t carStripeCommandResetCounter = 0;
volatile uint16_t carStripeCommandResetTime = 5000;
wsfxEffect_TypeDef *carStripeCurrentFx;


void carStripeResetWaitingForCommand(void);

void carStripe(void) {

	//initialize LED stripe
	WS2812B_init(&stripe1, &hspi1, CAR_STRIPE_LAST_LED);

	wsfx_init(&fxHueChange, &stripe1, 0, CAR_STRIPE_LAST_LED,
			wsfx_step_hueChange);
	wsfx_setValue(&fxHueChange, 255);
	wsfx_setPrescaler(&fxHueChange, 1);
	wsfx_setRepeat(&fxHueChange, WSFX_REPEAT_MODE_ON);
	wsfx_setSaturation(&fxHueChange, 230);

	wsfx_init(&fxKnight, &stripe1, 41, 55, wsfx_step_movingLight);
	wsfx_setColor(&fxKnight, 0);
	wsfx_setValue(&fxKnight, 255);
	wsfx_setPrescaler(&fxKnight, 50);
	wsfx_setRepeat(&fxKnight, WSFX_REPEAT_MODE_ON);

	wsfx_init(&fxGlow, &stripe1, 0, CAR_STRIPE_LAST_LED,
			wsfx_step_constantColor);
	wsfx_setValue(&fxGlow, 255);
	wsfx_setPrescaler(&fxGlow, 1);
	wsfx_setRepeat(&fxGlow, WSFX_REPEAT_MODE_ON);
	wsfx_setSaturation(&fxGlow, 220);

	wsfx_init(&fxPulse, &stripe1, 0, CAR_STRIPE_LAST_LED, wsfx_step_pulse);
	wsfx_setValue(&fxPulse, 255);
	wsfx_setSaturation(&fxPulse, 80);
	wsfx_setColor(&fxPulse, 240);
	wsfx_setPrescaler(&fxPulse, 1);

	wsfx_init(&fxBlinker, &stripe1, 0, CAR_STRIPE_LAST_LED,
			wsfx_step_blinker);
	wsfx_setPrescaler(&fxBlinker, 30);
	wsfx_setValue(&fxBlinker, 255);
	wsfx_setColor(&fxBlinker, 0);
	wsfx_setColorSecond(&fxBlinker, 240);
	wsfx_setSaturation(&fxBlinker, 255);


	wsfx_init(&fxStarting, &stripe1, 0, CAR_STRIPE_LAST_LED,
			wsfx_step_starting);
	wsfx_setPrescaler(&fxStarting, 1);
	wsfx_setValue(&fxStarting, 255);
	wsfx_setSaturation(&fxStarting, 50);
	wsfx_setColor(&fxStarting, 240);

	wsfx_init(&fxWarp, &stripe1, 0, CAR_STRIPE_LAST_LED,
			wsfx_step_warpHalf);
	wsfx_setPrescaler(&fxWarp, 2);
	wsfx_setValue(&fxWarp, 255);
	wsfx_setSaturation(&fxWarp, 0);
	wsfx_setRepeat(&fxWarp, WSFX_REPEAT_MODE_ON);

	//init receiver
	radio433_receiverInit(&radio, RADIO433_BARTS_FRAMELENGTH,
	RADIO433_BARTS_ID_SHIFT,
	RADIO433_BARTS_ID_MASK, RADIO433_BARTS_DATA_SHIFT,
	RADIO433_BARTS_DATA_MASK);
	//attach pin for receiver
	radio433_receiverAttach(&radio, RADIO_RX_GPIO_Port, RADIO_RX_Pin, &htim16);

	//init remote
	radio433_transmitterInit(&remote, 14854373, RADIO433_BARTS_FRAMELENGTH,
	RADIO433_BARTS_ID_SHIFT, RADIO433_BARTS_ID_MASK, RADIO433_BARTS_DATA_SHIFT,
	RADIO433_BARTS_DATA_MASK);

	//init transmitter
	//	radio433_transmitterInit(&transmitter, 14854373, RADIO433_BARTS_FRAMELENGTH,
	//	RADIO433_BARTS_ID_SHIFT, RADIO433_BARTS_ID_MASK, RADIO433_BARTS_DATA_SHIFT,
	//	RADIO433_BARTS_DATA_MASK);
	//	radio433_transmitterAttachGPIOandTimer(&transmitter, RADIO_TX_GPIO_Port,
	//	RADIO_TX_Pin, &htim17);

	while (1) {

		carStripeButtonData = radio433_receiverReadData(&radio, &remote);

		if (carStripeButtonData == RADIO433_BARTS_BUTTON_D) {

			if (carStripeCommand == 0)
				carStripeCommand = 1;

		}

		if (carStripeButtonData == RADIO433_BARTS_BUTTON_C) {

//			carStripeButtonReadPrescaler = 1;

			carStripeColor += 4;

			if (carStripeColor >= 360)
				carStripeColor = 0;

			wsfx_setColor(&fxKnight, carStripeColor);
			wsfx_setColor(&fxGlow, carStripeColor);

		}

		if (carStripeButtonData == RADIO433_BARTS_BUTTON_B) {

			if (carStripeCommand) {
				wsfx_stop(carStripeCurrentFx);
				carStripeEffectNumber++;
				if (carStripeEffectNumber > CAR_STRIPE_EFFECTS)
					carStripeEffectNumber = CAR_STRIPE_EFFECTS;

				carStripeCommand = 0;

			} else {
				carStripeStopFlag = 1;
			}

		}

		if (carStripeButtonData == RADIO433_BARTS_BUTTON_A) {

			if (carStripeCommand) {
				wsfx_stop(carStripeCurrentFx);
				carStripeEffectNumber--;
				if (carStripeEffectNumber < 0)
					carStripeEffectNumber = 0;

				carStripeCommand = 0;

			} else {
				carStripeStartFlag = 1;
			}

		}

		switch (carStripeEffectNumber) {
		case 0:
			carStripeCurrentFx = &fxPulse;
			break;
		case 1:
			carStripeCurrentFx = &fxKnight;
			break;
		case 2:
			carStripeCurrentFx = &fxStarting;
			break;
		case 3:
			carStripeCurrentFx = &fxWarp;
			break;
		case 4:
			carStripeCurrentFx = &fxGlow;
			break;
		case 5:
			carStripeCurrentFx = &fxHueChange;
			break;
		case 6:
			carStripeCurrentFx = &fxBlinker;
			break;
		}

		if (carStripeStartFlag) {
			wsfx_start(carStripeCurrentFx);
			carStripeStartFlag = 0;
		}

		if (carStripeStopFlag) {
			wsfx_stop(carStripeCurrentFx);
			carStripeStopFlag = 0;

		}

		carStripeButtonData = 0;

		wsfx_increment(7, &fxKnight, &fxGlow, &fxPulse, &fxBlinker, &fxStarting,
				&fxWarp, &fxHueChange);

	}

}
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin) {
	radio433_receiverCallbackEXTI(&radio, GPIO_Pin);
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin) {
	radio433_receiverCallbackEXTI(&radio, GPIO_Pin);
}

void carStripeResetWaitingForCommand(void) {

	if (carStripeCommand) {
		carStripeCommandResetCounter++;
		if (carStripeCommandResetCounter > carStripeCommandResetTime){
			carStripeCommand = 0;
			carStripeCommandResetCounter = 0;
		}

	}
}
