/*
 * ws281x.h
 *
 *  Created on: 19 févr. 2021
 *      Author: pjeanne
 */

#ifndef INC_WS281X_H_
#define INC_WS281X_H_

#include <stdint.h>
#include "main.h"

#define ONE  14
#define ZERO 7

#define LED_BYTES 3
#define LED_BITS LED_BYTES*8

#define NUM_LEDS 3

#define READBIT(byte, index) (((unsigned)(byte) >> (index)) & 1)

/*                     DATA
 * | BLANK |  BLUE(data[2])  |  GREEN(data[1]) |  RED(data[0])  */

typedef struct{
	union {
    uint8_t color[3];
    uint32_t data;
	};
}led;


void set_Led_color(uint8_t index,uint8_t  R, uint8_t G, uint8_t B);
void set_All_Leds_color(uint8_t  R, uint8_t G, uint8_t B);
uint32_t get_Led_value(uint8_t index);
void led_set_duty(uint8_t Dma_buff_pos,uint32_t led_state);
void led_update(void);
void led_set_reset_pulse(uint8_t Dma_buff_pos);
#endif /* INC_WS281X_H_ */
