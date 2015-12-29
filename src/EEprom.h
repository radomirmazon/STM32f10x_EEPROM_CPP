/*
 * EEprom.h
 *
 *  Created on: 28 gru 2015
 *      Author: radomir
 */

#ifndef EEPROM_H_
#define EEPROM_H_

#include "stm32f10x.h"


/* Define the STM32F10Xxx Flash page size depending on the used STM32 device */
#if defined (STM32F10X_LD) || defined (STM32F10X_MD)
  #define PAGE_SIZE  (uint16_t)0x400  /* Page size = 1KByte */
#elif defined (STM32F10X_HD) || defined (STM32F10X_CL)
  #define PAGE_SIZE  (uint16_t)0x800  /* Page size = 2KByte */
#endif

/* EEPROM start address in Flash */
#define EEPROM_START_ADDRESS    ((uint32_t)0x08010000) /* EEPROM emulation start address:
                                                  after 64KByte of used Flash memory */

#define SWAP_IS_FREE 		(uint16_t)0x0000
#define PAGE_IS_FREE		 	(uint16_t)0xFFFF

#define EEPROM_RESULT_OK 	0
#define EEPROM_RESULT_NOT_FOUND	1
#define EEPROM_RESULT_VADDR_INVALID	2

/**
 * This is programmabel format all space for EEPROM. If set 1, that all EEPROM will be erased.
 * You should set 0 after first run.
 */
#define FORMAT	1

class EEprom {
public:
	EEprom(uint8_t blockSize =0);
	virtual ~EEprom(){}

	uint8_t read(uint16_t VirtAddress, uint8_t* Data);
	uint8_t write(uint16_t VirtAddress, uint16_t* Data);

private:
	static uint32_t startAddressFreePage;
	static uint32_t startAddressSwapPage;
	uint32_t startAddress;
	/*final*/ uint8_t  blockSize;

	void fixSwapAddress();
	uint32_t getLastBlockAddress();
	uint32_t getPrevBlockAddress(uint32_t cursor);
	uint32_t getDataAddress(uint32_t cursor);
};

#endif /* EEPROM_H_ */
