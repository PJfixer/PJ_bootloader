/*
 * bootloader.h
 *
 *  Created on: 19 mai 2021
 *      Author: pjeanne
 */

#ifndef CORE_INC_BOOTLOADER_H_
#define CORE_INC_BOOTLOADER_H_

#define FLASH_PAGE_SIZE_USER (0x400)	//1kB datasheet
#define BOOTLOADER_MODE_SET_ADDRESS 0x801FFFC

#define APP1_START (0x08005000)			//Origin(datasheet) + Bootloader size (20kB)

#define UPLOAD_PACKET_SIZE 256
#define UPLOAD_FRAME_SIZE (UPLOAD_PACKET_SIZE+2) //we add '#' and '!' to delimit all packet




#define ERASE_FLASH_MEMORY "#ERASE_MEM"
#define FLASHING_START "#FLASH_START"
#define FLASHING_FINISH "#FLASH_FINISH!"
#define FLASHING_ABORT "#FLASH_ABORT!"

#include "main.h"
#include <string.h>

typedef enum
{
    JumpMode,
	FlashMode
} BootloaderMode;


typedef enum
{
    NO_ERROR,
	NO_APP
} Error_type;

typedef enum
{
    Unerased,
	Erased,
	Unlocked,
	Locked
} FlashStatus;

typedef void (application_t)(void);

typedef struct
{
    uint32_t		stack_addr;     // Stack Pointer
    application_t*	func_p;        // Program Counter
} JumpStruct;


uint32_t Flashed_offset;  // ???
uint32_t writed_packet;
FlashStatus flashStatus;
Error_type bootloader_error_state ;


void bootloaderInit();
int flashWord(uint32_t word);
uint32_t readWord(uint32_t address);
void eraseMemory(uint32_t nb_pageToerase);
void unlockFlashAndEraseMemory(uint32_t nb_pageToerase);
void lockFlash();
void jumpToApp();
void deinitEverything();
uint8_t string_compare(char array1[], char array2[], uint16_t length);
void errorBlink();
void serial_send(uint8_t * Buf, uint16_t length);
int messageHandler(uint8_t* Buf, uint16_t length);
int write_big_packet_flash(uint8_t * Buf);
void clear_flashmode_flag(void);


#endif /* CORE_INC_BOOTLOADER_H_ */
