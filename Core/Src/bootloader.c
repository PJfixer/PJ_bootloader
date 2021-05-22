/*
 * bootloader.c
 *
 *  Created on: 19 mai 2021
 *      Author: pjeanne
 */




#include "bootloader.h"

extern UART_HandleTypeDef huart3;

void bootloaderInit()
{
	Flashed_offset = 0; // we flash nothing yet set to 0
	flashStatus = Unerased; // we erase nothing yet set Unerased
	BootloaderMode bootloaderMode;

	if(readWord(BOOTLOADER_MODE_SET_ADDRESS) == 0xC0FFEE00)
	{
		__HAL_UART_ENABLE_IT(&huart3,UART_IT_RXNE);//enable uart  RX interupt here
		bootloaderMode = FlashMode;
	}
	else
	{
		bootloaderMode =JumpMode;
	}



	if(bootloaderMode == JumpMode)
	{

		/*	//Check if the application is there
			uint8_t emptyCellCount = 0;
			for(uint8_t i=0; i<10; i++)
			{
				if(readWord(APP1_START + (i*4)) == -1) // -1 stand for 0XFFFFFFFF
					emptyCellCount++;
			}

			if(emptyCellCount != 10)
				jumpToApp(APP1_START);
			else
				errorBlink();

		*/
	}
	else
	{

	}
}

void flashWord(uint32_t dataToFlash)
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
		  //CDC_Transmit_FS((uint8_t*)&"Flashing Error!\n", strlen("Flashing Error!\n"));
		  serial_send((uint8_t*)&"Flashing Error!\n", strlen("Flashing Error!\n"));
	  }else
	  {//Word Flash Successful
		  Flashed_offset += 4;
		  //CDC_Transmit_FS((uint8_t*)&"Flash: OK\n", strlen("Flash: OK\n"));
		  serial_send((uint8_t*)&"Flash: OK\n", strlen("Flash: OK\n"));

	  }
	}else
	{
	  //CDC_Transmit_FS((uint8_t*)&"Error: Memory not unlocked nor erased!\n",
		//	  strlen("Error: Memory not unlocked nor erased!\n"));
	  serial_send((uint8_t*)&"Error: Memory not unlocked nor erased!\n", strlen("Error: Memory not unlocked nor erased!\n"));
	}
}

uint32_t readWord(uint32_t address)
{
	uint32_t read_data;
	read_data = *(uint32_t*)(address);
	return read_data;
}

void eraseMemory()
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

	EraseInitStruct.NbPages = FLASH_BANK_SIZE/FLASH_PAGE_SIZE_USER;
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

void unlockFlashAndEraseMemory()
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

		EraseInitStruct.NbPages = FLASH_BANK_SIZE/FLASH_PAGE_SIZE_USER;
		uint32_t PageError;

		volatile HAL_StatusTypeDef status_erase;
		status_erase = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);

		if(status_erase != HAL_OK)
		{
			//errorBlink();
		}
	}

	flashStatus = Unlocked;
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

void messageHandler(uint8_t* Buf)
{
	if(string_compare((char*)Buf, ERASE_FLASH_MEMORY, strlen(ERASE_FLASH_MEMORY))
			&& flashStatus != Unlocked)
	{
		eraseMemory();
		//CDC_Transmit_FS((uint8_t*)&"Flash: Erased!\n", strlen("Flash: Erased!\n"));
		serial_send((uint8_t*)&"Flash: Erased!\n", strlen("Flash: Erased!\n"));
	}
	else if(string_compare((char*)Buf, FLASHING_START, strlen(FLASHING_START)))
	{
		unlockFlashAndEraseMemory();
		//CDC_Transmit_FS((uint8_t*)&"Flash: Unlocked!\n", strlen("Flash: Unlocked!\n"));
		serial_send((uint8_t*)&"Flash: Unlocked!\n", strlen("Flash: Unlocked!\n"));
	}
	else if(string_compare((char*)Buf, FLASHING_FINISH, strlen(FLASHING_FINISH))
			  && flashStatus == Unlocked)
	{
		lockFlash();
		//TODO : set BOOTLOADER_MODE_SET_ADDRESS to 0XFFFFFFFF to put bootloader in jumpmode after reset
		//CDC_Transmit_FS((uint8_t*)&"Flash: Success!\n", strlen("Flash: Success!\n"));
		serial_send((uint8_t*)&"Flash: Success! Rebooting ! \n", strlen("Flash: Success! Rebooting ! \n"));
		clear_flashmode_flag();
		HAL_Delay(100);
		NVIC_SystemReset();
	}
	else if(string_compare((char*)Buf, FLASHING_ABORT, strlen(FLASHING_ABORT))
			  && flashStatus == Unlocked)
	{
		lockFlash();
		eraseMemory();
		//CDC_Transmit_FS((uint8_t*)&"Flash: Aborted!\n", strlen("Flash: Aborted!\n"));
		serial_send((uint8_t*)&"Flash: Aborted!\n", strlen("Flash: Aborted!\n"));
	}
	else
	{
		//CDC_Transmit_FS((uint8_t*)&"Error: Incorrect step or unknown command!\n",
		//	  strlen("Error: Incorrect step or unknown command!\n"));
		serial_send((uint8_t*)&"Error: Incorrect step or unknown command!\n", strlen("Error: Incorrect step or unknown command!\n"));
	}
}
