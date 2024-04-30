/*
 * ask433.h
 *
 *  Created on: Apr 19, 2024
 *      Author: bartosz
 */

#ifndef RADIO433_H_
#define RADIO433_H_

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "tim.h"

/*
 * receiving LED
 */
#define RADIO433_RECEIVING_LED 0

/*
 * CNT time for whole bit
 */
#define RADIO433_CNT_BIT_TIME 199

/*
 * above this CNT value bit will be readed as 1, below -0
 */
#define RADIO433_CNT_BIT_STATE_TRESHOLD RADIO433_CNT_BIT_TIME / 2

/*
 * timer CNT values for sending Low/High bit
 */
#define RADIO433_CNT_BIT_H RADIO433_CNT_BIT_TIME / 4 * 3
#define RADIO433_CNT_BIT_L RADIO433_CNT_BIT_TIME / 4

/*
 * CNT TIME used for reset frame receiving trough idle state
 */
#define RADIO433_CNT_FRAME_INTERVAL_TRESHOLD 500

/*
 * CNT TIME used for frame separation while transmitting
 */
#define RADIO433_CNT_FRAME_INTERVAL 1500


/*
 * used for initialize not used Port/Pin before attachGPIO.
 * Also for check thant transmitter os remote or transmitter
 */
#define RADIO433_UNDEFINED 0

/*
 * number of frames to skip after data read
 * used to avoid receiving frames after successful receive
 */
#define RADIO433_SKIPPING_AFTER_READ 3

/*
 * barts standard
 */
#define RADIO433_BARTS_BUTTON_A 7
#define RADIO433_BARTS_BUTTON_B 11
#define RADIO433_BARTS_BUTTON_C 13
#define RADIO433_BARTS_BUTTON_D 14

#define RADIO433_BARTS_DATA_SHIFT 0
#define RADIO433_BARTS_DATA_MASK 0xff
#define RADIO433_BARTS_ID_SHIFT 8
#define RADIO433_BARTS_ID_MASK 0xffffff

#define RADIO433_BARTS_FRAMELENGTH 32



/*
 * 1527 STANDARD
 */
//BUTTONS
#define RADIO433_1527_BUTTON_A 8
#define RADIO433_1527_BUTTON_B 4
#define RADIO433_1527_BUTTON_C 2
#define RADIO433_1527_BUTTON_D 1

//SHIFTS AND MASKS
#define RADIO433_1527_DATA_SHIFT 1
#define RADIO433_1527_ID_SHIFT 5
#define RADIO433_1527_DATA_MASK 0xf
#define RADIO433_1527_ID_MASK 0xfffff

//FRAME LENGTH
#define RADIO433_1527_FRAMELENGTH 25

typedef struct {

	TIM_HandleTypeDef *timer;

	uint8_t frameLength;

	volatile uint32_t txDataFrame;  //overall rxData frame

	uint32_t id;
	uint32_t data;

	GPIO_TypeDef *txPort;
	uint16_t txPin;

	/*
	 * right shifts for id and data parts of frame
	 */
	uint8_t idShift;
	uint8_t dataShift;

	/*
	 * bit masks for id and data parts of frame
	 */
	uint32_t idBitMask;
	uint32_t dataBitMask;

} radio433_transmitterTypeDef;

typedef struct {

	TIM_HandleTypeDef *timer;
	uint8_t frameLength;

	volatile uint32_t rxDataFrame;  //overall rxData frame

	GPIO_TypeDef *rxPort;
	uint16_t rxPin;

#if RADIO433_RECEIVING_LED == 1

	GPIO_TypeDef *rxLEDPort;
	uint16_t rxLEDPin;

#endif



	volatile uint8_t actualReceivingBit; //used for bit counting trough receiving frame and detecting IDLE

	/*
	 * right shifts for id and data parts of frame
	 */
	uint8_t idShift;
	uint8_t dataShift;

	/*
	 * bit masks for id and data parts of frame
	 */
	uint32_t idBitMask;
	uint32_t dataBitMask;

	volatile uint32_t id; //id pulled from rxFrame
	volatile uint32_t data; //data pulled from rxFrame

	/*
	 * used to block receiving frames after successful read
	 * in read sets to RADIO433_SKIPPING_AFTER_READ, and decrementing ever successful read.
	 * receiver callback saves new frames only after downcouting to 0
	 * counter is immediately set to 0 on failed attempt to read data from transmitter- uint32_t radio433_receiverReadData()
	 */
	uint8_t skippingReceivedCounter; //flag set after run radio433_receiverReadData()

} radio433_receiverTypeDef;

void radio433_receiverInit(radio433_receiverTypeDef *radio,
		TIM_HandleTypeDef *timer, uint8_t frameLength, uint8_t idShift,
		uint32_t idBitMask, uint8_t dataShift, uint32_t dataBitMask);

void radio433_receiverAttachGPIO(radio433_receiverTypeDef *radio,
		GPIO_TypeDef *port, uint16_t pin);

#if RADIO433_RECEIVING_LED == 1
void radio433_receiverAttachRxLED(radio433_receiverTypeDef *radio,
		GPIO_TypeDef *port, uint16_t pin);
#endif

uint32_t radio433_receiverReadDataCheck(radio433_receiverTypeDef *radio,
		radio433_transmitterTypeDef *transmitter);

void radio433_receiverCallbackEXTI(radio433_receiverTypeDef *radio,
		uint16_t pin);

void radio433_transmitterInit(radio433_transmitterTypeDef *transmitter,
		uint32_t id, uint8_t frameLength, uint8_t idShift, uint32_t idBitMask,
		uint8_t dataShift, uint32_t dataBitMask);

void radio433_transmitterAttachGPIOandTimer(
		radio433_transmitterTypeDef *transmitter, GPIO_TypeDef *port,
		uint16_t pin, TIM_HandleTypeDef *timer);

void radio433_transmitterSendData(radio433_transmitterTypeDef *transmitter,
		uint32_t data);

#endif /* RADIO433_H_ */
