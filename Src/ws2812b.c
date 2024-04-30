/*
 * ws2712b.c
 *
 *  Created on: Apr 18, 2024
 *      Author: bartosz
 *
 *      note:
 *      set spi to 8Mbps
 *
 *      if DMA_MODE 1:
 *      set channel for SPI*_TX memory to periph, low priority
 *		mode normal, data width- Byte, increment memory address
 *
 */

#define DMA_MODE 1

#include "ws2812b.h"

void _WS2812B_bitsToBytes(uint8_t *destination, uint8_t *source,
		uint16_t sourceSize, uint16_t rightShift, uint8_t high, int8_t low);

void WS2812B_init(ws2812_Stripe_TypeDef *stripe, SPI_HandleTypeDef *spi,
		uint16_t ledCount) {
	stripe->spi = spi;
	stripe->ledCount = ledCount;
	stripe->colorBuffer = malloc(stripe->ledCount * 3);
	memset(stripe->colorBuffer, 0x0, stripe->ledCount * 3);
}

void WS2812B_setLedColorRGB(ws2812_Stripe_TypeDef *stripe, uint16_t id,
		uint32_t color) {

	if (id >= stripe->ledCount || id < 0)
		return;

	uint8_t red = (color >> 16) & 0xff;
	uint8_t green = (color >> 8) & 0xff;
	uint8_t blue = color & 0xff;

	uint32_t convertedColor = blue << 16 | red << 8 | green;

	memcpy(&stripe->colorBuffer[id * 3], &convertedColor, 3);

}

void WS2812B_setLedColorHSV(ws2812_Stripe_TypeDef *stripe, uint16_t id,
		uint16_t hueDegree, uint8_t saturation, uint8_t value) {

	uint8_t red, green, blue;
	float redPrime, greenPrime, bluePrime;

	if (hueDegree >= 360)
		hueDegree = 360;

	float value_f = (float) value / 255;
	float saturation_f = (float) saturation / 255;

	//chroma- high level of color
	//x- ascending/descending level of color
	float chroma = value_f * saturation_f;
	float x = chroma * (1 - fabs(fmodf(((float) hueDegree / 60), 2) - 1)); //changing
	float m = value_f - chroma;

	if (0 <= hueDegree && hueDegree < 60) {
		redPrime = chroma; //red high
		greenPrime = x; //green changing
		bluePrime = 0; //blue zero
	} else if (60 <= hueDegree && hueDegree < 120) {
		redPrime = x;
		greenPrime = chroma;
		bluePrime = 0;
	} else if (120 <= hueDegree && hueDegree < 180) {
		redPrime = 0;
		greenPrime = chroma;
		bluePrime = x;
	} else if (180 <= hueDegree && hueDegree < 240) {
		redPrime = 0;
		greenPrime = x;
		bluePrime = chroma;
	} else if (240 <= hueDegree && hueDegree < 300) {
		redPrime = x;
		greenPrime = 0;
		bluePrime = chroma;
	} else if (300 <= hueDegree && hueDegree < 360) {
		redPrime = chroma;
		greenPrime = 0;
		bluePrime = x;
	}

	red = (redPrime + m) * 255;
	green = (greenPrime + m) * 255;
	blue = (bluePrime + m) * 255;

	uint32_t color = red << 16 | green << 8 | blue;

	WS2812B_setLedColorRGB(stripe, id, color);

}

void WS2812B_Refresh(ws2812_Stripe_TypeDef *stripe) {

	uint16_t colorBufferSize = stripe->ledCount * 3;
	uint16_t sendBufferSize = (colorBufferSize * 8) + 40; // one byte for each bit in each LED + 40 for reset

	bitState sendBuffer[sendBufferSize];

	_WS2812B_bitsToBytes(sendBuffer, stripe->colorBuffer, colorBufferSize, 40,
			WS2812B_HIGH, WS2812B_LOW);

#if DMA_MODE == 0
	HAL_SPI_Transmit(stripe->spi, sendBuffer, sendBufferSize, 200); // Additional 3 for reset signal
#else

	if (HAL_SPI_Transmit_DMA(stripe->spi, sendBuffer, sendBufferSize) == HAL_OK)
		while (HAL_DMA_GetState(stripe->spi->hdmatx) != HAL_DMA_STATE_READY)
			;

#endif

}

/*
 * covert series of bits to series of bytes
 *
 * right shift is shift of destination data. set for 5 cause 5 empty bytes
 * on the beginning of destination and shifts the rest
 */
void _WS2812B_bitsToBytes(uint8_t *destination, uint8_t *source,
		uint16_t sourceSize, uint16_t rightShift, uint8_t high, int8_t low) {

	uint16_t destinationSize = (sourceSize * 8) + rightShift;

	//clear destination
	memset(destination, 0x0, destinationSize);

	for (uint16_t i = 0; i < sourceSize; i++) {

		for (uint16_t j = 0; j < 8; j++) {

			uint16_t destinationOffset = (i * 8) + j;

			if (source[i] & (1 << (7 - j)))
				destination[destinationOffset + rightShift] = high;
			else
				destination[destinationOffset + rightShift] = low;

		}
	}

}

