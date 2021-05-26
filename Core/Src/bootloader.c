/*
 * bootloader.c
 *
 *  Created on: 19 mai 2021
 *      Author: pjeanne
 */




#include "bootloader.h"
#include "ws281x.h"

extern UART_HandleTypeDef huart3;
extern TIM_HandleTypeDef htim1;



void bootloaderInit()
{
	Flashed_offset = 0; // we flash nothing yet set to 0
	flashStatus = Unerased; // we erase nothing yet set Unerased


	if(readWord(BOOTLOADER_MODE_SET_ADDRESS) == 0xC0FFEE00)
	{

		bootloaderMode = FlashMode;
	}
	else
	{
		bootloaderMode =JumpMode;
	}



	if(bootloaderMode == JumpMode)
	{

			//Check if the application is there
			uint8_t emptyCellCount = 0;
			for(uint8_t i=0; i<10; i++)
			{
				if(readWord(APP1_START + (i*4)) == -1) // -1 stand for 0XFFFFFFFF
					emptyCellCount++;
			}

			if(emptyCellCount != 10)
			{

				jumpToApp(APP1_START);
			}
			else
			{
				errorJump();
			}


	}
	else
	{

	}
}


int flashWord(uint32_t dataToFlash)
{
	if(flashStatus == Unlocked)
	{
	  volatile HAL_StatusTypeDef status;
	  uint8_t flash_attempt = 0;
	  uint32_t address;
	  do
	  {

		  address = APP1_START + Flashed_offset;
		  status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, dataToFlash);
		  flash_attempt++;
	  }while(status != HAL_OK && flash_attempt < 10 && dataToFlash == readWord(address));
	  if(status != HAL_OK)
	  {
		  return -1;

	  }else
	  {//Word Flash Successful
		  Flashed_offset += 4;

		  return 0;
	  }
	}else
	{
	  serial_send((uint8_t*)&"Error: Memory not unlocked nor erased!\n", strlen("Error: Memory not unlocked nor erased!\n"));
	  return -2;
	}
}

uint32_t readWord(uint32_t address)
{
	uint32_t read_data;
	read_data = *(uint32_t*)(address);
	return read_data;
}

void eraseMemory(uint32_t nb_pageToerase)
{
	/* Unock the Flash to enable the flash control register access *************/
	while(HAL_FLASH_Unlock()!=HAL_OK)
		while(HAL_FLASH_Lock()!=HAL_OK);//Weird fix attempt

	/* Allow Access to option bytes sector */
	while(HAL_FLASH_OB_Unlock()!=HAL_OK)
		while(HAL_FLASH_OB_Lock()!=HAL_OK);//Weird fix attempt

	/* Fill EraseInit structure*/
	FLASH_EraseInitTypeDef EraseInitStruct;
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;

	EraseInitStruct.PageAddress = APP1_START;

	EraseInitStruct.NbPages = nb_pageToerase;
	uint32_t PageError;

	volatile HAL_StatusTypeDef status_erase;
	status_erase = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);

	/* Lock the Flash to enable the flash control register access *************/
	while(HAL_FLASH_Lock()!=HAL_OK)
		while(HAL_FLASH_Unlock()!=HAL_OK);//Weird fix attempt

	/* Lock Access to option bytes sector */
	while(HAL_FLASH_OB_Lock()!=HAL_OK)
		while(HAL_FLASH_OB_Unlock()!=HAL_OK);//Weird fix attempt

	if(status_erase != HAL_OK)
		//errorBlink();
	flashStatus = Erased;
	Flashed_offset = 0;
}

void unlockFlashAndEraseMemory(uint32_t nb_pageToerase)
{
	/* Unock the Flash to enable the flash control register access *************/
	while(HAL_FLASH_Unlock()!=HAL_OK)
		while(HAL_FLASH_Lock()!=HAL_OK);//Weird fix attempt

	/* Allow Access to option bytes sector */
	while(HAL_FLASH_OB_Unlock()!=HAL_OK)
		while(HAL_FLASH_OB_Lock()!=HAL_OK);//Weird fix attempt

	if(flashStatus != Erased)
	{
		/* Fill EraseInit structure*/
		FLASH_EraseInitTypeDef EraseInitStruct;
		EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;

		EraseInitStruct.PageAddress = APP1_START;

		EraseInitStruct.NbPages = nb_pageToerase;
		uint32_t PageError;

		volatile HAL_StatusTypeDef status_erase;
		status_erase = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);

		if(status_erase != HAL_OK)
		{
			serial_send((uint8_t*)&"error during erasing\n", strlen("error during erasing\n"));
		}
	}

	flashStatus = Unlocked;
	Flashed_offset = 0;
}

void lockFlash()
{
	/* Lock the Flash to enable the flash control register access *************/
	while(HAL_FLASH_Lock()!=HAL_OK)
		while(HAL_FLASH_Unlock()!=HAL_OK);//Weird fix attempt

	/* Lock Access to option bytes sector */
	while(HAL_FLASH_OB_Lock()!=HAL_OK)
		while(HAL_FLASH_OB_Unlock()!=HAL_OK);//Weird fix attempt

	flashStatus = Locked;
}

void jumpToApp(const uint32_t address)
{
	const JumpStruct* vector_p = (JumpStruct*)address;

	deinitEverything();

	/* let's do The Jump! */
    /* Jump, used asm to avoid stack optimization */
    asm("msr msp, %0; bx %1;" : : "r"(vector_p->stack_addr), "r"(vector_p->func_p));
}

void deinitEverything()
{

	//-- reset peripherals to guarantee flawless start of user application
	//TODO : deinit hardware
	__HAL_UART_DISABLE_IT(&huart3,UART_IT_RXNE);
	HAL_UART_DeInit(&huart3);
	HAL_TIM_PWM_DeInit(&htim1);
	HAL_NVIC_DisableIRQ(DMA1_Channel2_IRQn);
	__HAL_RCC_DMA1_CLK_DISABLE();
	__HAL_RCC_GPIOA_CLK_DISABLE();
	__HAL_RCC_GPIOB_CLK_DISABLE();
	__HAL_RCC_GPIOD_CLK_DISABLE();
	HAL_RCC_DeInit();
	HAL_DeInit();
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;
}

void clear_flashmode_flag(void)
{
	/* Unock the Flash to enable the flash control register access *************/
		while(HAL_FLASH_Unlock()!=HAL_OK)
			while(HAL_FLASH_Lock()!=HAL_OK);//Weird fix attempt

		/* Allow Access to option bytes sector */
		while(HAL_FLASH_OB_Unlock()!=HAL_OK)
			while(HAL_FLASH_OB_Lock()!=HAL_OK);//Weird fix attempt

		/* Fill EraseInit structure*/
		FLASH_EraseInitTypeDef EraseInitStruct;
		EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;

		EraseInitStruct.PageAddress = BOOTLOADER_MODE_SET_ADDRESS;

		EraseInitStruct.NbPages = 1;
		uint32_t PageError;

		volatile HAL_StatusTypeDef status_erase;
		status_erase = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);

		/* Lock the Flash to enable the flash control register access *************/
		while(HAL_FLASH_Lock()!=HAL_OK)
			while(HAL_FLASH_Unlock()!=HAL_OK);//Weird fix attempt

		/* Lock Access to option bytes sector */
		while(HAL_FLASH_OB_Lock()!=HAL_OK)
			while(HAL_FLASH_OB_Unlock()!=HAL_OK);//Weird fix attempt

		if(status_erase != HAL_OK)
		{
			//errorBlink();
		}
}

uint8_t string_compare(char array1[], char array2[], uint16_t length)
{
	 uint8_t comVAR=0, i;
	 for(i=0;i<length;i++)
	   	{
	   		  if(array1[i]==array2[i])
	   	  		  comVAR++;
	   	  	  else comVAR=0;
	   	}
	 if (comVAR==length)
		 	return 1;
	 else 	return 0;
}

void serial_send(uint8_t * Buf, uint16_t length)
{
	 HAL_UART_Transmit(&huart3,Buf,length,50);
}

int write_big_packet_flash(uint8_t * Buf)
{

	uint8_t *start_addr = &Buf[1]; // we skip "#"
	uint16_t buffer_idx = 0;
	 for(int i =0;i<(UPLOAD_PACKET_SIZE/4);i++)
	 {
		/* uint8_t a = (start_addr[3+buffer_idx]);
		 uint8_t b = (start_addr[2+buffer_idx]) ;
		 uint8_t c =  (start_addr[1+buffer_idx]);
		 uint8_t d = start_addr[0+buffer_idx]; */

		 uint32_t dataToFlash = (start_addr[3+buffer_idx]<<24) +
													  (start_addr[2+buffer_idx]<<16) +
													  (start_addr[1+buffer_idx]<<8) +
													  start_addr[0+buffer_idx];//32bit Word contains 4 Bytes


		 if(flashWord(dataToFlash) == -1)
		 {
			return -1;
		 }

		/* if(i == 0)
		 {
			 uint8_t debug_purpose_only =1;
		 }*/
		 buffer_idx += 4;
	 }
	 return 0;
}

void errorJump(void)
{
	while(1)
	{
		 set_All_Leds_color(255,0,0);
		 led_update();
		 HAL_Delay(500);
		 set_All_Leds_color(0,0,0);
		 led_update();
		 HAL_Delay(500);
	}
}


int messageHandler(uint8_t* Buf, uint16_t length)
{
	uint32_t page_to_erase_nb = 0;
	if((length > 6 ) && (length < 18) ) // we assume all commands have a length greater than 6
	{

//length = length -1; // remove the '!'

		if(string_compare((char*)Buf, ERASE_FLASH_MEMORY, strlen(ERASE_FLASH_MEMORY))
				&& flashStatus != Unlocked)
		{
			page_to_erase_nb = (Buf[length-2]<<24) +(Buf[length-3]<<16) +(Buf[length-4]<<8) +Buf[length-5];//32bit Word contains 4 Bytes
			eraseMemory(page_to_erase_nb);
			//CDC_Transmit_FS((uint8_t*)&"Flash: Erased!\n", strlen("Flash: Erased!\n"));
			serial_send((uint8_t*)&"Flash: Erased!\n", strlen("Flash: Erased!\n"));
			return 1;
		}
		else if(string_compare((char*)Buf, FLASHING_START, strlen(FLASHING_START)))
		{
			page_to_erase_nb = (Buf[length-2]<<24) +(Buf[length-3]<<16) +(Buf[length-4]<<8) +Buf[length-5];//32bit Word contains 4 Bytes
			unlockFlashAndEraseMemory(page_to_erase_nb);
			//CDC_Transmit_FS((uint8_t*)&"Flash: Unlocked!\n", strlen("Flash: Unlocked!\n"));
 			writed_packet = 0;
			serial_send((uint8_t*)&"Flash: Unlocked!\n", strlen("Flash: Unlocked!\n"));
			return 1;
		}
		else if(string_compare((char*)Buf, FLASHING_FINISH, strlen(FLASHING_FINISH))
				  && flashStatus == Unlocked)
		{
			lockFlash();
			//TODO : set BOOTLOADER_MODE_SET_ADDRESS to 0XFFFFFFFF to put bootloader in jumpmode after reset
			//CDC_Transmit_FS((uint8_t*)&"Flash: Success!\n", strlen("Flash: Success!\n"));
			serial_send((uint8_t*)&"Flash: Success! Rebooting ! \n", strlen("Flash: Success! Rebooting ! \n"));
			clear_flashmode_flag();
			for(uint8_t i = 0;i < 5;i++)
			{
				set_All_Leds_color(0,255,0);
				led_update();
				HAL_Delay(100);
				set_All_Leds_color(0,255,0);
			    led_update();
				HAL_Delay(100);
			}
			NVIC_SystemReset();

		}
		else if(string_compare((char*)Buf, FLASHING_ABORT, strlen(FLASHING_ABORT))
				  && flashStatus == Unlocked)
		{
			lockFlash();
			eraseMemory(page_to_erase_nb);
			//CDC_Transmit_FS((uint8_t*)&"Flash: Aborted!\n", strlen("Flash: Aborted!\n"));
			serial_send((uint8_t*)&"Flash: Aborted!\n", strlen("Flash: Aborted!\n"));
			return 1;
		}
		/*else
		{
			//CDC_Transmit_FS((uint8_t*)&"Error: Incorrect step or unknown command!\n",
			//	  strlen("Error: Incorrect step or unknown command!\n"));
			serial_send((uint8_t*)&"Error: Incorrect step or unknown command!\n", strlen("Error: Incorrect step or unknown command!\n"));
			return 0;
		}*/
	}
	return 0;
}
