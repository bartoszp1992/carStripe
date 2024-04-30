/*
 * ask433.c
 *
 *  Created on: Apr 19, 2024
 *      Author: bartosz
 *
 *      library to handle 433 ASK receivers, transmitters and remotes
 *
 *      note:
 *      set timer prescaler for counting to RADIO433_BIT_TIME in 1,6ms(example)
 *     	!!set arr to max
 *
 *     	example for 32MHz timer CLK:
 *     	ARR- 65535
 *     	PSC- 255
 *
 *     	example for 64MHz timer CLK:
 *     	ARR- 65535
 *     	PSC- 511
 *
 *		set proper RADIO433_CNT values!
 *
 *		set rxPin as EXTI_INPUT with rising/falling detection
 *
 *		1527 frame description
 *
 *		0		1	2	3	4	5	6	7	8	9	10	11	12	13	14	15	16	17	18	19	20	21	22	23	24
 *		|beg(H)	|id of transmitter															|B1	|B2	|B3	|B4	|end(L)|
 *
 *		H- logical high, long pulse
 *		L- logical low, short pulse
 *		B0-B3- buttons (H- pressed, L- unpressed)
 *
 *		BARTS frame description
 *		|24bit id | 8bit data | or
 *		|24bit id | 4bit additional data | 4bit button data(pressed low)|
 *
 */

#include <radio433.h>

/*
 * initialize radio. remember to run also radio433_receiverAttachGPIO()
 * @param: radio struct
 * @param: timer handler
 * @param: lenght of frame(25 for 1527 coding)
 * @param: right shift for ID of transmitter(5 for 1527 coding)
 * @param: bit mask for ID of transmitter(0xfffff for 1527 encoding)
 * @param: right shift for data from transmitter(1 for 1527 coding)
 * @param: bit mask for data from transmitter(0xf for 1527 encoding)
 */
void radio433_receiverInit(radio433_receiverTypeDef *radio,
		TIM_HandleTypeDef *timer, uint8_t frameLength, uint8_t idShift,
		uint32_t idBitMask, uint8_t dataShift, uint32_t dataBitMask) {

	radio->timer = timer;
	radio->frameLength = frameLength;

	radio->rxDataFrame = 0;

	radio->rxPort = RADIO433_UNDEFINED;
	radio->rxPin = RADIO433_UNDEFINED;

	radio->actualReceivingBit = 0;

	radio->idShift = idShift;
	radio->dataShift = dataShift;

	radio->idBitMask = idBitMask;
	radio->dataBitMask = dataBitMask;

	radio->skippingReceivedCounter = 0;

	HAL_TIM_Base_Start(radio->timer);

}

/*
 * attaches GPIO to receiver. Dont forget to run it after receiver init
 * @param: radio struct
 * @param: port
 * @param: pin
 */
void radio433_receiverAttachGPIO(radio433_receiverTypeDef *radio,
		GPIO_TypeDef *port, uint16_t pin) {
	radio->rxPort = port;
	radio->rxPin = pin;
}

#if RADIO433_RECEIVING_LED == 1
void radio433_receiverAttachRxLED(radio433_receiverTypeDef *radio,
		GPIO_TypeDef *port, uint16_t pin) {
	radio->rxLEDPort = port;
	radio->rxLEDPin = pin;
}
#endif

uint32_t _radio433_pullData(uint32_t dataFrame, uint8_t shift, uint32_t bitMask) {

	return (dataFrame >> shift) & bitMask;

}

uint32_t _radio433_pushData(uint32_t data, uint8_t dataShift, uint32_t id,
		uint8_t idShift) {
	return (data << dataShift | id << idShift);
}

/*
 * returns received data and clear it if id of radios are identical
 * @param: radio struct
 * @param: transmitter struct(for compare id)
 * @retval: data or RADIO433_ERR(if transmitter havent matched id)
 */
uint32_t radio433_receiverReadDataCheck(radio433_receiverTypeDef *radio,
		radio433_transmitterTypeDef *transmitter) {

	uint32_t retval;

	if (radio->id == transmitter->id) {
		retval = radio->data;

		//clear output
		radio->data = 0;
		radio->id = 0;

		//reset skipping counter
		radio->skippingReceivedCounter = RADIO433_SKIPPING_AFTER_READ;

	} else {
		radio->skippingReceivedCounter = 0;
		retval = 0;
	}

	return retval;

}

/*
 * run in EXTI from receiver pin
 *
 * @param: radio struct
 * @param: pin received from EXTI(GPIO_Pin)- for compare
 */
void radio433_receiverCallbackEXTI(radio433_receiverTypeDef *radio,
		uint16_t pin) {

	if (pin == radio->rxPin) {

		if (HAL_GPIO_ReadPin(radio->rxPort, radio->rxPin) == GPIO_PIN_SET) { // rising edge

			if (radio->timer->Instance->CNT
					> RADIO433_CNT_FRAME_INTERVAL_TRESHOLD) {
				radio->actualReceivingBit = 0;

#if RADIO433_RECEIVING_LED == 1
				HAL_GPIO_WritePin(radio->rxLEDPort, radio->rxLEDPin,
						GPIO_PIN_SET);
#endif
			}
			radio->timer->Instance->CNT = 0;

		} else { //falling edge

			if (radio->timer->Instance->CNT > RADIO433_CNT_BIT_STATE_TRESHOLD) {
				radio->rxDataFrame |=
						1
								<< ((radio->frameLength - 1)
										- radio->actualReceivingBit);
			} else {
				radio->rxDataFrame &=
						~(1
								<< ((radio->frameLength - 1)
										- radio->actualReceivingBit));
			}

			radio->actualReceivingBit++;

			if (radio->actualReceivingBit >= radio->frameLength) {
				radio->actualReceivingBit = 0;

				//decrement skipping counter
				if (radio->skippingReceivedCounter > 0)
					radio->skippingReceivedCounter--;

				//pull data only when the appropriate number of frames have been skipped after succesful read
				if (radio->skippingReceivedCounter == 0) {

					radio->id = _radio433_pullData(radio->rxDataFrame,
							radio->idShift, radio->idBitMask);
					radio->data = _radio433_pullData(radio->rxDataFrame,
							radio->dataShift, radio->dataBitMask);

				}

#if RADIO433_RECEIVING_LED == 1
				HAL_GPIO_WritePin(radio->rxLEDPort, radio->rxLEDPin,
						GPIO_PIN_RESET);
#endif

			}

		}

	}
}

/*
 * initialize transmitter or remote
 * @param: transmitter struct
 * @param: id
 * @param: frame length
 * @param: idShift
 * @param: idBitMask
 * @param: data shift
 * @param: data bit mask
 */
void radio433_transmitterInit(radio433_transmitterTypeDef *transmitter,
		uint32_t id, uint8_t frameLength, uint8_t idShift, uint32_t idBitMask,
		uint8_t dataShift, uint32_t dataBitMask) {

	transmitter->frameLength = frameLength;
	transmitter->txDataFrame = 0;
	transmitter->id = id;
	transmitter->data = 0;

	transmitter->txPort = RADIO433_UNDEFINED;
	transmitter->txPin = RADIO433_UNDEFINED;

	transmitter->idShift = idShift;
	transmitter->dataShift = dataShift;

	transmitter->idBitMask = idBitMask;
	transmitter->dataBitMask = dataBitMask;

}

/*
 * attach GPIO and timer to transmitter
 * dont use in remote.
 */
void radio433_transmitterAttachGPIOandTimer(
		radio433_transmitterTypeDef *transmitter, GPIO_TypeDef *port,
		uint16_t pin, TIM_HandleTypeDef *timer) {

	transmitter->txPort = port;
	transmitter->txPin = pin;
	transmitter->timer = timer;

	if (transmitter->timer->State != HAL_TIM_STATE_BUSY)
		HAL_TIM_Base_Start(transmitter->timer);

}

/*
 * send data with transmitter
 */
void radio433_transmitterSendData(radio433_transmitterTypeDef *transmitter,
		uint32_t data) {

	transmitter->data = data;

	transmitter->txDataFrame = _radio433_pushData(transmitter->data,
			transmitter->dataShift, transmitter->id, transmitter->idShift);

	//start frame
	for (uint8_t i = 0; i < transmitter->frameLength; i++) { //repeat for each bit

		HAL_GPIO_WritePin(transmitter->txPort, transmitter->txPin,
				GPIO_PIN_SET); //set bit H
		transmitter->timer->Instance->CNT = 0; //reset timer

		if (transmitter->txDataFrame >> (transmitter->frameLength - 1 - i)
				& 1) { //high bit
			while (transmitter->timer->Instance->CNT < RADIO433_CNT_BIT_H)
				; //wait for counting high state

		} else { //low bit
			while (transmitter->timer->Instance->CNT < RADIO433_CNT_BIT_L)
				; //wait for counting low state
		}

		HAL_GPIO_WritePin(transmitter->txPort, transmitter->txPin,
				GPIO_PIN_RESET); //reset pin
		while (transmitter->timer->Instance->CNT < RADIO433_CNT_BIT_TIME)
			; //wait for counting to bit end

	}

	//reset
	transmitter->timer->Instance->CNT = 0; //reset timer
	while (transmitter->timer->Instance->CNT < RADIO433_CNT_FRAME_INTERVAL)
		;

}
