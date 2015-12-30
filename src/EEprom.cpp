/*
 * EEprom.cpp
 *
 *  Created on: 28 gru 2015
 *      Author: radomir
 */

#include "EEprom.h"
#include "stm32f10x_flash.h"

uint32_t EEprom::startAddressSwapPage = EEPROM_START_ADDRESS;

uint32_t EEprom::startAddressFreePage = EEPROM_START_ADDRESS + PAGE_SIZE;

EEprom::EEprom(uint8_t blockSizeInByte) {
	this->blockSize = blockSizeInByte;
	/**
	 * block Size include
	 * <1B, 30B>
	 * TODO: think about max. Why I put 30B in my notes ?? It is realy limited ?
	 */
	if (blockSizeInByte > 30) {
		this->blockSize = 30;
	}
	if (blockSizeInByte < 1) {
		this->blockSize = 1;
	}
	this->startAddress = EEprom::startAddressFreePage;
	EEprom::startAddressFreePage += PAGE_SIZE;
	fixSwapAddress();
#if FORMAT == 1
	FLASH_ErasePage(startAddress);
#endif
}

/**
 * Detect swap page and fix address.
 */
void EEprom::fixSwapAddress() {
	uint16_t pageStatus = (*(__IO uint16_t*) startAddress);
	if (pageStatus == SWAP_IS_FREE) {
		uint32_t temp = startAddress;
		startAddress = EEprom::startAddressSwapPage;
		EEprom::startAddressSwapPage = temp;
	}
}

/* Return value: (0: variable exist, 1: variable doesn't exist, 2: address is not valid) */
uint8_t EEprom::read(uint16_t virtAddress, uint8_t* data) {
	if (virtAddress == SWAP_IS_FREE || virtAddress == PAGE_IS_FREE) {
		return EEPROM_RESULT_VADDR_INVALID;
	}
	uint8_t result = EEPROM_RESULT_NOT_FOUND;

	uint32_t cursor = getLastBlockAddress(startAddress);

	while (cursor >= startAddress) {
		uint16_t blockVirtualAddress = *(__IO uint16_t*) cursor;
		if (blockVirtualAddress == virtAddress) {
			for (int i = 0; i < blockSize; i = i + 2) {
				uint16_t data16_t = *(__IO uint16_t*) (getDataAddress(cursor)
						+ i);
				data[i] = (uint8_t) (data16_t >> 8);
				if (i + 1 < blockSize) {
					data[i + 1] = (uint8_t) (data16_t & 0x00ff);
				}
			}
			result = EEPROM_RESULT_OK;
			break;
		}
		cursor = getPrevBlockAddress(cursor);
	}
	return result;
}

/* Return value: (0: variable writed, 4: page is full, 2: address is not valid, 3: flash operation faild) */
uint8_t EEprom::write(uint16_t virtAddress, uint16_t* data) {
	if (virtAddress == SWAP_IS_FREE || virtAddress == PAGE_IS_FREE) {
		return 2;
	}

	uint32_t cursor = getLastBlockAddress(startAddress);

	while (cursor >= startAddress) {
		uint16_t blockVirtualAddress = *(__IO uint16_t*) cursor;
		if (blockVirtualAddress == PAGE_IS_FREE) {
			FLASH_Status status = FLASH_ProgramHalfWord(cursor, virtAddress);
			if (status != FLASH_COMPLETE) {
				return EEPROM_RESULT_FLASH_FAILD;
			}
			for (int i = 0; i < blockSize; i = i + 2) {
				uint16_t dataToSave = ((uint16_t) data[i]) << 8;
				if (i + 1 < blockSize) {
					dataToSave = dataToSave & ((uint16_t) data[i + 1]);
				}
				FLASH_Status status = FLASH_ProgramHalfWord(
						getDataAddress(cursor), dataToSave);
				if (status != FLASH_COMPLETE) {
					return EEPROM_RESULT_FLASH_FAILD;
				}
			}
			break;
		}
		cursor = getPrevBlockAddress(cursor);
	}

	return cleanUp();
}

uint32_t EEprom::getLastBlockAddress(uint32_t startAddress) {
	uint8_t fixBlockSize = blockSize;
	if (fixBlockSize % 2 == 1) {
		fixBlockSize++;
	}
	uint8_t realBlockSize = fixBlockSize + 2; /* 2 byte for virtual address*/
	uint8_t capacity = PAGE_SIZE / realBlockSize;
	return startAddress + realBlockSize * capacity - realBlockSize;
}

uint32_t EEprom::getPrevBlockAddress(uint32_t cursor) {
	uint8_t fixBlockSize = blockSize;
	if (fixBlockSize % 2 == 1) {
		fixBlockSize++;
	}
	return cursor - fixBlockSize + 2;
}

uint32_t EEprom::getDataAddress(uint32_t cursor) {
	return cursor + 2;
}

uint8_t EEprom::cleanUp() {
	uint16_t swapStatus = *(__IO uint16_t*) startAddressSwapPage;
	if (swapStatus != SWAP_IS_FREE) {
		return EEPROM_RESULT_SWAP_IS_BUSY;
	}

	FLASH_ErasePage(startAddressSwapPage);

	uint32_t cursor = getLastBlockAddress(startAddress);
	uint32_t swapCursor = getLastBlockAddress(startAddressSwapPage);
	uint8_t result = EEPROM_RESULT_OK;

	//TODO: transfer...

	//check cleanup result
	if (cursor == swapCursor) {
		result = EEPROM_RESULT_FULL;
	}

	//change to valid page
	cursor = startAddressSwapPage;
	startAddressSwapPage = startAddress;
	startAddress = cursor;

	//mark new swap is free
	FLASH_Status status = FLASH_ProgramHalfWord(startAddressSwapPage, SWAP_IS_FREE);
	if (status != FLASH_COMPLETE) {
		return EEPROM_RESULT_FLASH_FAILD;
	}
	return result;
}
