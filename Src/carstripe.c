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
wsfxEffect_TypeDef fxHue;
wsfxEffect_TypeDef fxPulse;
wsfxEffect_TypeDef fxBlinker;
wsfxEffect_TypeDef fxStarting;
wsfxEffect_TypeDef fxWarp;

radio433_receiverTypeDef radio;
radio433_transmitterTypeDef remote;
//radio433_transmitterTypeDef transmitter;

void carStripeReadButton(void);
uint32_t carStripeButtonReadPrescaler = 200;
uint32_t carStripeButtonReadMasterCounter;
uint32_t carStripeButtonData;
uint8_t carStripeEffectNumber;
uint16_t carStripeColor;
uint8_t carStripeStartFlag = 0;
uint8_t carStripeStopFlag = 0;
wsfxEffect_TypeDef *carStripeCurrentFx;

void carStripe(void) {

	//initialize LED stripe
	WS2812B_init(&stripe1, &hspi1, 118);

	//initialize and config kreffect
	wsfx_init(&fxKnight, &stripe1, 52, 66, wsfx_step_movingLight);
	wsfx_setColor(&fxKnight, 0);
	wsfx_setValue(&fxKnight, 255);
	wsfx_setPrescaler(&fxKnight, 50);
	wsfx_setRepeat(&fxKnight, WSFX_REPEAT_MODE_ON);

	wsfx_init(&fxHue, &stripe1, 0, 117, wsfx_step_hueChange);
	wsfx_setValue(&fxHue, 255);
	wsfx_setPrescaler(&fxHue, 1);

	wsfx_init(&fxPulse, &stripe1, 0, 117, wsfx_step_pulse);
	wsfx_setValue(&fxPulse, 255);
	wsfx_setSaturation(&fxPulse, 80);
	wsfx_setColor(&fxPulse, 240);
	wsfx_setPrescaler(&fxPulse, 1);

	wsfx_init(&fxBlinker, &stripe1, 0, 117, wsfx_step_blinker);
	wsfx_setPrescaler(&fxBlinker, 30);
	wsfx_setValue(&fxBlinker, 255);
	wsfx_setColor(&fxBlinker, 0);
	wsfx_setColorSecond(&fxBlinker, 240);
	wsfx_setSaturation(&fxBlinker, 255);

	wsfx_init(&fxStarting, &stripe1, 0, 117, wsfx_step_starting);
	wsfx_setPrescaler(&fxStarting, 1);
	wsfx_setValue(&fxStarting, 255);
	wsfx_setSaturation(&fxStarting, 50);
	wsfx_setColor(&fxStarting, 240);

	wsfx_init(&fxWarp, &stripe1, 0, 117, wsfx_step_warp);
	wsfx_setPrescaler(&fxWarp, 2);
	wsfx_setValue(&fxWarp, 255);
	wsfx_setSaturation(&fxWarp, 0);
	wsfx_setRepeat(&fxWarp, WSFX_REPEAT_MODE_ON);

	//init receiver
	radio433_receiverInit(&radio, &htim16, RADIO433_BARTS_FRAMELENGTH,
	RADIO433_BARTS_ID_SHIFT,
	RADIO433_BARTS_ID_MASK, RADIO433_BARTS_DATA_SHIFT,
	RADIO433_BARTS_DATA_MASK);
	//attach pin for receiver
	radio433_receiverAttachGPIO(&radio, RADIO_RX_GPIO_Port, RADIO_RX_Pin);

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

	carStripeEffectNumber = 3;

	while (1) {

		if (carStripeButtonData == RADIO433_BARTS_BUTTON_D) {

			if (carStripeCurrentFx->flow.repeat == WSFX_REPEAT_MODE_ON
					&& carStripeCurrentFx->flow.state
							== WSFX_EFFECT_STATE_RUNNING) {//dont allow change if running

			} else {
				carStripeButtonReadPrescaler = 200;

				carStripeEffectNumber++;

				if (carStripeEffectNumber > CAR_STRIPE_EFFECTS)
					carStripeEffectNumber = 0;

			}

		}

		if (carStripeButtonData == RADIO433_BARTS_BUTTON_C) {

			carStripeButtonReadPrescaler = 1;

			carStripeColor += 2;

			if (carStripeColor >= 360)
				carStripeColor = 0;

			wsfx_setColor(&fxKnight, carStripeColor);

		}

		if (carStripeButtonData == RADIO433_BARTS_BUTTON_B) {

			carStripeStopFlag = 1;

		}

		if (carStripeButtonData == RADIO433_BARTS_BUTTON_A) {

			carStripeStartFlag = 1;

		}

		switch (carStripeEffectNumber) {
		case 0:
			carStripeCurrentFx = &fxStarting;
			break;
		case 1:
			carStripeCurrentFx = &fxHue;
			break;
		case 2:
			carStripeCurrentFx = &fxPulse;
			break;
		case 3:
			carStripeCurrentFx = &fxKnight;
			break;
		case 4:
			carStripeCurrentFx = &fxBlinker;
			break;
		case 5:
			carStripeCurrentFx = &fxWarp;
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

		wsfx_increment(6, &fxKnight, &fxHue, &fxPulse, &fxBlinker, &fxStarting,
				&fxWarp);

	}

}
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin){
	radio433_receiverCallbackEXTI(&radio, GPIO_Pin);
}


void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin){
	radio433_receiverCallbackEXTI(&radio, GPIO_Pin);
}


void carStripeReadButton(void) {

	if (carStripeButtonReadMasterCounter % carStripeButtonReadPrescaler == 0) {
		carStripeButtonData = radio433_receiverReadDataCheck(&radio, &remote);
	}

	carStripeButtonReadMasterCounter++;
}

