/*
 * carstripe.h
 *
 *  Created on: Apr 30, 2024
 *      Author: bartosz
 */

#ifndef CARSTRIPE_H_
#define CARSTRIPE_H_

#include <string.h>
#include "ws2812b.h"
#include "wsfx.h"
#include "radio433.h"

#define CAR_STRIPE_EFFECTS 6
#define CAR_STRIPE_LEDS_COUNTER 8

void carStripe(void);


#endif /* CARSTRIPE_H_ */
