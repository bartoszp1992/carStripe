/*
 * wsfx.h
 *
 *  Created on: Apr 19, 2024
 *      Author: bartosz
 */

#ifndef WSFX_H_
#define WSFX_H_

#include <stdarg.h>
#include <stdint.h>
#include <ws2812b.h>



typedef enum {
	WSFX_EFFECT_STATE_STOP = 0, WSFX_EFFECT_STATE_RUNNING
} wsfxEffectState;

typedef enum {
	WSFX_REPEAT_MODE_OFF = 0, WSFX_REPEAT_MODE_ON
} wsfxRepeatMode;



typedef enum {
	WSFX_DIRECTION_FORE = 0, WSFX_DIRECTION_BACK
} wsfxDirection;

/*
 * used with effects function
 */
typedef struct{

	volatile uint16_t counter;
	wsfxDirection direction;
	ws2812_Stripe_TypeDef *stripe;
	uint16_t beginLED;
	uint16_t endLED;

	//stores max steps. counted by effect function only!
	uint16_t steps;

	uint16_t hue;
	uint16_t hue2;

	uint8_t value;

	uint8_t saturation;

	//preinitialized/automatic values
	wsfxEffectState state; //running/stop


	//user options
	wsfxRepeatMode repeat; //is effect repeatable?

	uint16_t prescaler; //effect prescaler

	uint8_t additionalData[4]; //additional data for some fx'es

} wsfxEffectFlow_TypeDef;

typedef struct {

	//flow parameters
	wsfxEffectFlow_TypeDef flow;


	//step function pointer
	void (*stepFunction)(wsfxEffectFlow_TypeDef *flow);


} wsfxEffect_TypeDef;

void wsfx_increment(uint8_t effectsToRun, ...);


void wsfx_init(wsfxEffect_TypeDef *effect, ws2812_Stripe_TypeDef *stripe,
		uint16_t beginLED, uint16_t endLED,
		void (*stepFunction)(wsfxEffectFlow_TypeDef *flow));

void wsfx_setColor(wsfxEffect_TypeDef *effect, uint16_t hue);
void wsfx_setValue(wsfxEffect_TypeDef *effect, uint8_t value);
void wsfx_setRepeat(wsfxEffect_TypeDef *effect, wsfxRepeatMode repeat);
void wsfx_setDirection(wsfxEffect_TypeDef *effect, wsfxDirection direction);
void wsfx_setPrescaler(wsfxEffect_TypeDef *effect, uint16_t prescaler);
void wsfx_setSaturation(wsfxEffect_TypeDef *effect, uint8_t saturation);
void wsfx_setColorSecond(wsfxEffect_TypeDef *effect, uint16_t hue);


void wsfx_start(wsfxEffect_TypeDef *effect);
void wsfx_stop(wsfxEffect_TypeDef *effect);
void wsfx_interruptCallback(void);

void wsfx_step_movingLight(wsfxEffectFlow_TypeDef *flow);
void wsfx_step_hueChange(wsfxEffectFlow_TypeDef *flow);
void wsfx_step_pulse(wsfxEffectFlow_TypeDef *flow);
void wsfx_step_blinker(wsfxEffectFlow_TypeDef *flow);
void wsfx_step_turn(wsfxEffectFlow_TypeDef *flow);
void wsfx_step_flyingLight(wsfxEffectFlow_TypeDef *flow);
void wsfx_step_starting(wsfxEffectFlow_TypeDef *flow);
void wsfx_step_constantColor(wsfxEffectFlow_TypeDef *flow);
void wsfx_step_warp(wsfxEffectFlow_TypeDef *flow);

#endif /* WSFX_H_ */
