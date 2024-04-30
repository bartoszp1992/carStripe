/*
 * ws2112b.h
 *
 *  Created on: Apr 18, 2024
 *      Author: bartosz
 */

#ifndef WS2812B_H_
#define WS2812B_H_

#include "spi.h"
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "string.h"

#define WS2812B_TIMEOUT 500


typedef enum {
	WS2812B_LOW = 0b10000000, WS2812B_HIGH = 0b11111000
} bitState;

typedef struct {

	SPI_HandleTypeDef *spi;
	uint8_t *colorBuffer;
	uint16_t ledCount;

} ws2812_Stripe_TypeDef;

void WS2812B_init(ws2812_Stripe_TypeDef *stripe, SPI_HandleTypeDef *spi,
		uint16_t ledCount);
void WS2812B_setLedColorRGB(ws2812_Stripe_TypeDef *stripe, uint16_t id,
		uint32_t color);
void WS2812B_setLedColorHSV(ws2812_Stripe_TypeDef *stripe, uint16_t id,
		uint16_t hue, uint8_t saturation, uint8_t value);
void WS2812B_Refresh(ws2812_Stripe_TypeDef *stripe);

#endif /* WS2812B_H_ */
