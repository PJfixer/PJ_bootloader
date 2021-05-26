/*
 * ws281x.c
 *
 *  Created on: 19 févr. 2021
 *      Author: pjeanne
 */

#include "ws281x.h"

extern TIM_HandleTypeDef htim1;


led led_strip[NUM_LEDS];



uint8_t led_number = 0;
uint8_t reset_pulse = 0;
uint32_t DMA_Double_buff[((LED_BITS)*2)]; // 2leds = 48 bits = 48 duty_cycle



void set_Led_color(uint8_t index,uint8_t  R, uint8_t G, uint8_t B)
{
	led_strip[index].color[1] = R;
	led_strip[index].color[0] = G;
	led_strip[index].color[2] = B;

}


void set_All_Leds_color(uint8_t  R, uint8_t G, uint8_t B)
{
	for(int i =0;i<NUM_LEDS;i++)
	{
		led_strip[i].color[1] = R;
		led_strip[i].color[0] = G;
		led_strip[i].color[2] = B;
	}

}


uint32_t get_Led_value(uint8_t index)
{
	return led_strip[index].data ;
}

void led_set_duty(uint8_t Dma_buff_pos,uint32_t led_state)
{
	int i = 0;
	int DMA_buff_idx = 0;
	if(Dma_buff_pos == 0) // on place sur la premiere moitié du buffer
	{
		for( i =23;i>=0;i--)
		{
			if(READBIT(((led_state)),i)  ==  1) // si le bit est a 1
			{
				DMA_Double_buff[DMA_buff_idx] = ONE;
				DMA_buff_idx++;
			}
			else // si a 0
			{
				DMA_Double_buff[DMA_buff_idx] = ZERO;
				DMA_buff_idx++;
			}
		}
	}
	else // on place sur la deuxieme
	{
		for( i =23;i>=0;i--)
		{
			if(READBIT(((led_state)),i)  ==  1) // si le bit est a 1
			{
				DMA_Double_buff[LED_BITS+DMA_buff_idx] = ONE;
				DMA_buff_idx++;
			}
			else // si a 0
			{
				DMA_Double_buff[LED_BITS+DMA_buff_idx] = ZERO;
				DMA_buff_idx++;
			}
		}
	}

}


void led_set_reset_pulse(uint8_t Dma_buff_pos)
{


	int i = 0;
	int DMA_buff_idx = 0;
	if(Dma_buff_pos == 0) // on place sur la premiere moitié du buffer
	{
		for( i =23;i>=0;i--)
		{

			DMA_Double_buff[DMA_buff_idx] = 0;
			DMA_buff_idx++;
		}
	}
	else // on place sur la deuxieme
	{
		for( i =23;i>=0;i--)
		{

				DMA_Double_buff[LED_BITS+DMA_buff_idx] = 0;
				DMA_buff_idx++;

		}
	}

}


void led_update(void)
{

	reset_pulse = 0;
	htim1.Instance->CCR1 = 0; //reset duty cycle
	led_number = 0;
	//chargement d'un temps a l'etat bas ( reset pulse ) puis de la premiere led
	led_set_reset_pulse(0);
	led_set_reset_pulse(1);


	//start DMA tranfer

	 while (HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, DMA_Double_buff,48) != HAL_OK)
	 {

	 }



}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) // this interupt callack  override ST HAL weak function
{
	if(reset_pulse == 1)
	{
		 while (HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_1) != HAL_OK)
		 {

		 }

	}


	if(led_number >= NUM_LEDS) // si c'est la derniere led qui vient d'tre envoyé alors on recrit un reset pulse (des 0)
	{
		reset_pulse = 1;
		led_set_reset_pulse(1);

	}
	else // sinon on ecrit la suivante a la fin  du double buffer (DMA_Double_buff)
	{
		led_set_duty(1,led_strip[led_number].data);
		led_number++;
	}




}

void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim)  // this interupt callack  override ST HAL weak function
{
	if(reset_pulse == 1)
		{
			 while (HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_1) != HAL_OK)
			 {

			 }

		}


		if(led_number >= NUM_LEDS) // si c'est la derniere led qui vient d'tre envoyé alors on recrit un reset pulse (des 0)
		{
			reset_pulse = 1;
			led_set_reset_pulse(0);


		}
		else // sinon on ecrit la suivante au debut   du double buffer (DMA_Double_buff)
		{
			led_set_duty(0,led_strip[led_number].data);
			led_number++;
		}



}




