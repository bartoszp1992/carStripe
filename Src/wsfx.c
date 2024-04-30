/*
 * wsfx.c
 *
 *  Created on: Apr 19, 2024
 *      Author: bartosz
 */

#include "wsfx.h"
#include "ws2812b.h"

volatile enum {
	WSFX_STOP = 0, WSFX_RUN
} wsfx_flagRun = WSFX_STOP;

/*
 * master counter. Increments every run
 */
volatile uint32_t wsfx_mainCounter = 0;
;

//private functions declaration
void _groupAction(ws2812_Stripe_TypeDef *stripe, int first, int last,
		uint16_t hue, uint8_t saturation, uint8_t value);

void _groupTurnOff(ws2812_Stripe_TypeDef *stripe, int first, int last);

/*
 * run this function in main loop, to run as often as possible
 * @param: number of arguments
 * @params: effects to increment(wsfxEffect_TypeDef*)
 */
void wsfx_increment(uint8_t effectsToRun, ...) {

	if (wsfx_flagRun == WSFX_RUN) {

		wsfx_mainCounter++;

		va_list ap;
		va_start(ap, effectsToRun);

		for (uint8_t i = 0; i < effectsToRun; i++) { //run this for each effect

			/*
			 * get first/next struct
			 */
			wsfxEffect_TypeDef *currentEffect = va_arg(ap, wsfxEffect_TypeDef*);

			if (currentEffect->flow.state == WSFX_EFFECT_STATE_RUNNING //if effect is running
			&& (wsfx_mainCounter % currentEffect->flow.prescaler == 0)) { //and main couter can divide by prescaler

				currentEffect->stepFunction(&currentEffect->flow); //run function

				currentEffect->flow.counter++; //increment effect coutner

				if (currentEffect->flow.counter >= currentEffect->flow.steps) { //if step is too large
					currentEffect->flow.counter = 0;	//reset counter
					if (currentEffect->flow.repeat == WSFX_REPEAT_MODE_OFF) {//if repeat is off
						currentEffect->flow.state = WSFX_EFFECT_STATE_STOP; //stop effect
						for (int i = currentEffect->flow.beginLED;
								i <= currentEffect->flow.endLED; i++) {
							WS2812B_setLedColorRGB(currentEffect->flow.stripe,
									i, 0x0);
						}
						WS2812B_Refresh(currentEffect->flow.stripe);

					}

				}
			}

		}

		va_end(ap);

		wsfx_flagRun = WSFX_STOP;

	}
}

/*
 * initialize effect
 *
 * @param: effect struct
 * @param: repeat mode WSFX_REPEAD_MODE_OFF/ON
 * @param: direction WSFX_DIRECTION_FORE/BACK
 * @param: prescaler(larger=slower)
 * @param: stripe struct
 * @param: first LED to handle effect
 * @param: last led to handle effect
 * @param: step function: one of wsfx_effect_*() function
 *
 */
void wsfx_init(wsfxEffect_TypeDef *effect, ws2812_Stripe_TypeDef *stripe,
		uint16_t beginLED, uint16_t endLED,
		void (*stepFunction)(wsfxEffectFlow_TypeDef *flow)) {

	effect->flow.state = WSFX_EFFECT_STATE_STOP;
	effect->flow.counter = 0;
	effect->flow.steps = 0;

	//default values
	effect->flow.repeat = WSFX_REPEAT_MODE_OFF;
	effect->flow.direction = WSFX_DIRECTION_FORE;
	effect->flow.prescaler = 20;
	effect->flow.saturation = 255;
	effect->flow.hue = 0;
	effect->flow.hue2 = 180;
	effect->flow.value = 20;

	effect->flow.stripe = stripe;
	effect->flow.beginLED = beginLED;
	effect->flow.endLED = endLED;

	effect->stepFunction = stepFunction;

}

/*
 * set main color for effect
 * @param: effect struct
 * @param: hue 0-359
 */
void wsfx_setColor(wsfxEffect_TypeDef *effect, uint16_t hue) {
	effect->flow.hue = hue;
}

/*
 * set main value(brightness) for effect
 * @param: effect
 * @param: value(0-255)
 */
void wsfx_setValue(wsfxEffect_TypeDef *effect, uint8_t value) {
	effect->flow.value = value;
}

void wsfx_setRepeat(wsfxEffect_TypeDef *effect, wsfxRepeatMode repeat) {
	effect->flow.repeat = repeat;
}

void wsfx_setDirection(wsfxEffect_TypeDef *effect, wsfxDirection direction) {
	effect->flow.direction = direction;
}

void wsfx_setPrescaler(wsfxEffect_TypeDef *effect, uint16_t prescaler) {
	effect->flow.prescaler = prescaler;
}

void wsfx_setSaturation(wsfxEffect_TypeDef *effect, uint8_t saturation) {
	effect->flow.saturation = saturation;
}

void wsfx_setColorSecond(wsfxEffect_TypeDef *effect, uint16_t hue) {
	effect->flow.hue2 = hue;
}

/*
 * start effect
 */
void wsfx_start(wsfxEffect_TypeDef *effect) {
	effect->flow.state = WSFX_EFFECT_STATE_RUNNING;
}

/*
 * stop effect
 */
void wsfx_stop(wsfxEffect_TypeDef *effect) {
	effect->flow.state = WSFX_EFFECT_STATE_STOP;
	effect->flow.counter = 0;

	_groupTurnOff(effect->flow.stripe, effect->flow.beginLED,
			effect->flow.endLED);
	WS2812B_Refresh(effect->flow.stripe);

}

/*
 * run this function in interrupt callback
 */
void wsfx_interruptCallback(void) {
	wsfx_flagRun = WSFX_RUN;
}

void _groupAction(ws2812_Stripe_TypeDef *stripe, int first, int last,
		uint16_t hue, uint8_t saturation, uint8_t value) {

	for (int i = first; i <= last; i++) {
		WS2812B_setLedColorHSV(stripe, i, hue, saturation, value);
	}

}

void _groupTurnOff(ws2812_Stripe_TypeDef *stripe, int first, int last) {

	for (int i = first; i <= last; i++) {
		WS2812B_setLedColorRGB(stripe, i, 0x0);
	}

}

void wsfx_step_movingLight(wsfxEffectFlow_TypeDef *flow) {

	int leds = flow->endLED - flow->beginLED + 1;

	flow->steps = (leds * 2);

	int currentLED;

	uint8_t glowDirection = 0;

	if (flow->counter < leds) {

		currentLED = flow->counter + flow->beginLED;
		glowDirection = 0;

	} else {

		currentLED = flow->steps - flow->counter - 1 + flow->beginLED;
		glowDirection = 1;

	}

	if (flow->direction == WSFX_DIRECTION_BACK)
		currentLED = leds - 1 - currentLED;

	_groupTurnOff(flow->stripe, flow->beginLED, flow->endLED);

	WS2812B_setLedColorHSV(flow->stripe, currentLED, flow->hue,
			flow->saturation, flow->value);

	//glow
	if (glowDirection == 0) {
		if (currentLED > flow->beginLED)
			WS2812B_setLedColorHSV(flow->stripe, currentLED - 1, flow->hue,
					flow->saturation, flow->value / 2);
		if (currentLED > flow->beginLED + 1)
			WS2812B_setLedColorHSV(flow->stripe, currentLED - 2, flow->hue,
					flow->saturation, flow->value / 4);
	} else {
		if (currentLED < flow->endLED)
			WS2812B_setLedColorHSV(flow->stripe, currentLED + 1, flow->hue,
					flow->saturation, flow->value / 2);
		if (currentLED +2 < flow->endLED + 1)
			WS2812B_setLedColorHSV(flow->stripe, currentLED + 2, flow->hue,
					flow->saturation, flow->value / 4);
	}

	WS2812B_Refresh(flow->stripe);

}

void wsfx_step_hueChange(wsfxEffectFlow_TypeDef *flow) {

	flow->steps = 360;

	_groupAction(flow->stripe, flow->beginLED, flow->endLED, flow->counter,
			flow->saturation, flow->value);

	WS2812B_Refresh(flow->stripe);

}

void wsfx_step_pulse(wsfxEffectFlow_TypeDef *flow) {

	flow->steps = 50;

	uint8_t actualValue = 0;

	if (flow->counter < 5) {
		uint8_t divider = 6 - flow->counter;
		actualValue = flow->value / divider;
	}

	else if (flow->counter == 5) {
		actualValue = flow->value;
	} else {
		actualValue = flow->value / flow->counter;
	}

	_groupAction(flow->stripe, flow->beginLED, flow->endLED, flow->hue,
			flow->saturation, actualValue);

	WS2812B_Refresh(flow->stripe);

}

void wsfx_step_blinker(wsfxEffectFlow_TypeDef *flow) {

	flow->steps = 16;

	int leds = flow->endLED - flow->beginLED + 1;

	int midLED = leds / 2;

	if (flow->counter & 1) { //for odd turn off LEDs

		_groupTurnOff(flow->stripe, flow->beginLED, flow->endLED);

	} else {

		if (flow->counter < flow->steps / 2) { // light first color

			_groupAction(flow->stripe, flow->beginLED, flow->beginLED + midLED,
					flow->hue, flow->saturation, flow->value);

		} else { //light second color

			_groupAction(flow->stripe, flow->beginLED + midLED, flow->endLED,
					flow->hue2, flow->saturation, flow->value);

		}
	}

	WS2812B_Refresh(flow->stripe);

}

void wsfx_step_turn(wsfxEffectFlow_TypeDef *flow) {

	flow->steps = flow->endLED - flow->beginLED + 1;

	_groupTurnOff(flow->stripe, flow->beginLED, flow->endLED);

	if (flow->direction == WSFX_DIRECTION_FORE)
		_groupAction(flow->stripe, flow->beginLED, flow->counter, flow->hue,
				flow->saturation, flow->value);
	else
		_groupAction(flow->stripe, flow->endLED - flow->counter, flow->endLED,
				flow->hue, flow->saturation, flow->value);

	WS2812B_Refresh(flow->stripe);

}

void wsfx_step_flyingLight(wsfxEffectFlow_TypeDef *flow) {

	int leds = flow->endLED - flow->beginLED + 1;

	flow->steps = leds;

	int currentLED;

	currentLED = flow->counter + flow->beginLED;

	if (flow->direction == WSFX_DIRECTION_BACK)
		currentLED = leds - 1 - currentLED;

	_groupTurnOff(flow->stripe, flow->beginLED, flow->endLED);

	WS2812B_setLedColorHSV(flow->stripe, currentLED, flow->hue,
			flow->saturation, flow->value);

	WS2812B_Refresh(flow->stripe);

}

void wsfx_step_starting(wsfxEffectFlow_TypeDef *flow) {

	int leds = flow->endLED - flow->beginLED + 1;

	flow->steps = (leds) * 2 + 20;

	int currentLED1, currentLED2;

	if (flow->counter < (flow->steps - 20) / 2) { //first stage

		_groupTurnOff(flow->stripe, flow->beginLED, flow->endLED);
		currentLED1 = flow->beginLED + flow->counter;
		currentLED2 = flow->endLED - flow->counter;

		WS2812B_setLedColorHSV(flow->stripe, currentLED1, flow->hue,
				flow->saturation, flow->value / 4);
		WS2812B_setLedColorHSV(flow->stripe, currentLED2, flow->hue,
				flow->saturation, flow->value / 4);

	} else if (flow->counter < flow->steps - 20 - leds / 2) { //second stage

		currentLED1 = flow->beginLED + (flow->counter - flow->endLED);
		currentLED2 = flow->endLED - (flow->counter - flow->endLED);

		WS2812B_setLedColorHSV(flow->stripe, currentLED1, flow->hue,
				flow->saturation, flow->value / 4);
		WS2812B_setLedColorHSV(flow->stripe, currentLED2, flow->hue,
				flow->saturation, flow->value / 4);

	} else if (flow->counter <= flow->steps - 20) { //third stage

		currentLED1 = flow->beginLED + (flow->counter - flow->endLED - 1);
		currentLED2 = flow->endLED - (flow->counter - flow->endLED - 1);

		WS2812B_setLedColorHSV(flow->stripe, currentLED1, flow->hue,
				flow->saturation, flow->value);
		WS2812B_setLedColorHSV(flow->stripe, currentLED2, flow->hue,
				flow->saturation, flow->value);

	} else { //last stage

		uint8_t divider = flow->counter - (flow->steps - 20);

		_groupAction(flow->stripe, flow->beginLED, flow->endLED, flow->hue,
				flow->saturation, flow->value / divider);

	}

	WS2812B_Refresh(flow->stripe);

}

void wsfx_step_constantColor(wsfxEffectFlow_TypeDef *flow) {

	flow->steps = 20;

	_groupAction(flow->stripe, flow->beginLED, flow->endLED, flow->hue,
			flow->saturation, flow->value);

	WS2812B_Refresh(flow->stripe);

}

void wsfx_step_warp(wsfxEffectFlow_TypeDef *flow) {

	int leds = flow->endLED - flow->beginLED + 1;

	flow->steps = (leds) / 2;

	int currentLED1, currentLED2;

	_groupTurnOff(flow->stripe, flow->beginLED, flow->endLED);
	currentLED1 = flow->beginLED + flow->counter;
	currentLED2 = flow->endLED - flow->counter;

	WS2812B_setLedColorHSV(flow->stripe, currentLED1, flow->hue,
			flow->saturation, flow->value / 4);
	WS2812B_setLedColorHSV(flow->stripe, currentLED2, flow->hue,
			flow->saturation, flow->value / 4);

	WS2812B_Refresh(flow->stripe);

}
