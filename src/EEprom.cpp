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

	fixBlockSize = blockSize;
	if (fixBlockSize % 2 == 1) {
		fixBlockSize++;
	}

	this->startAddress = EEprom::startAddressFreePage;
	EEprom::startAddressFreePage += PAGE_SIZE;
	fixSwapAddress();
	FLASH_Unlock();

#if FORMAT == 1
	FLASH_ErasePage(startAddress);
	if ((*(__IO uint16_t*) EEprom::startAddressSwapPage) != SWAP_IS_FREE) {
		FLASH_ErasePage(EEprom::startAddressSwapPage);
		FLASH_ProgramHalfWord(EEprom::startAddressSwapPage, SWAP_IS_FREE);
	}
#endif

	this->freeBlockaddress = getFreeBlockAddress();
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

	uint32_t cursor;

	if (freeBlockaddress == 0) {
		cursor = getLastBlockAddress(startAddress);
	} else {
		cursor = getPrevBlockAddress(freeBlockaddress);
	}

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
uint8_t EEprom::write(uint16_t virtAddress, uint8_t* data) {
	if (virtAddress == SWAP_IS_FREE || virtAddress == PAGE_IS_FREE) {
		return EEPROM_RESULT_VADDR_INVALID;
	}

	if (freeBlockaddress == 0) {
		return EEPROM_RESULT_FULL;
	}

	if (getNextBlockAddress(freeBlockaddress) >= startAddress + PAGE_SIZE) {
		uint8_t status = tryCleanUp();
		if (status != EEPROM_RESULT_OK) {
			return status;
		}
	}

	FLASH_Status status = FLASH_ProgramHalfWord(freeBlockaddress, virtAddress);
	if (status != FLASH_COMPLETE) {
		return EEPROM_RESULT_FLASH_FAILD;
	}
	for (int i = 0; i < blockSize; i = i + 2) {
		uint16_t dataToSave = ((uint16_t) data[i]) << 8;
		if (i + 1 < blockSize) {
			dataToSave = dataToSave | ((uint16_t) data[i + 1]);
		}
		FLASH_Status status = FLASH_ProgramHalfWord(
				getDataAddress(freeBlockaddress) + i, dataToSave);
		if (status != FLASH_COMPLETE) {
			return EEPROM_RESULT_FLASH_FAILD;
		}
	}

	freeBlockaddress = getNextBlockAddress(freeBlockaddress);
	if (freeBlockaddress < startAddress + PAGE_SIZE) {
		return EEPROM_RESULT_OK;
	}
	return tryCleanUp();
}

uint8_t EEprom::tryCleanUp() {
	freeBlockaddress = 0;
	if (checkCapacity() == EEPROM_RESULT_FULL) {
		return EEPROM_RESULT_FULL;
	}
	uint8_t result = cleanUp();
	if (result != EEPROM_RESULT_OK) {
		return result;
	}

	return EEPROM_RESULT_OK;
}

uint32_t EEprom::getLastBlockAddress(uint32_t startAddress) {
	uint8_t realBlockSize = fixBlockSize + 2; /* 2 byte for virtual address*/
	uint16_t capacity = PAGE_SIZE / realBlockSize;
	return startAddress + realBlockSize * capacity - realBlockSize;
}

uint32_t EEprom::getPrevBlockAddress(uint32_t cursor) {
	return cursor - (fixBlockSize + 2);
}

uint32_t EEprom::getNextBlockAddress(uint32_t cursor) {
	return cursor + fixBlockSize + 2;
}

uint32_t EEprom::getDataAddress(uint32_t cursor) {
	return cursor + 2;
}

uint8_t EEprom::cleanUp() {
	uint16_t swapStatus = *(__IO uint16_t*) startAddressSwapPage;
	if (swapStatus != SWAP_IS_FREE && swapStatus != PAGE_IS_FREE) {
		return EEPROM_RESULT_SWAP_IS_BUSY;
	}

	FLASH_ErasePage(startAddressSwapPage);

	uint32_t cursor = getLastBlockAddress(startAddress);
	uint32_t swapCursor = startAddressSwapPage;
	uint8_t result = EEPROM_RESULT_OK;

	uint32_t finishSwapAddress;
	uint16_t finishSwapData;

	while (cursor >= startAddress) {
		uint16_t blockVirtualAddress = *(__IO uint16_t*) cursor;
		if (blockVirtualAddress != PAGE_IS_FREE) {
			if (searchVirtualAddressInSwap(blockVirtualAddress) == 0) {
				//transfering....
				if (swapCursor == startAddressSwapPage) {
					/** This is first 16bit of Swap.
					 * First 16bit determine that the Swap is not in use.
					 * After erase in flash is 0xffff, that mean
					 * the Swap is not in use. When the power is off
					 * during data are transferring, swap is still free
					 * and all data are in page.
					 */
					finishSwapAddress = swapCursor;
					finishSwapData = blockVirtualAddress;
				} else {
					FLASH_Status status = FLASH_ProgramHalfWord(swapCursor,
							blockVirtualAddress);
					if (status != FLASH_COMPLETE) {
						return EEPROM_RESULT_FLASH_FAILD;
					}
				}
				for (int i = 0; i < blockSize; i = i + 2) {
					uint16_t data = *(__IO uint16_t*) (cursor + i);
					FLASH_ProgramHalfWord(getDataAddress(swapCursor) + i, data);
				}
				swapCursor = getNextBlockAddress(swapCursor);
			}
		}
		cursor = getPrevBlockAddress(cursor);
	}

	//check cleanup result
	if (swapCursor > getLastBlockAddress(startAddressSwapPage)) {
		result = EEPROM_RESULT_FULL;
	}

	FLASH_Status status = FLASH_ProgramHalfWord(finishSwapAddress,
			finishSwapData);
	if (status != FLASH_COMPLETE) {
		return EEPROM_RESULT_FLASH_FAILD;
	}
	//change to valid page
	cursor = startAddressSwapPage;
	startAddressSwapPage = startAddress;
	startAddress = cursor;
	freeBlockaddress = getFreeBlockAddress();

	//mark new swap is free
	status = FLASH_ProgramHalfWord(startAddressSwapPage,
	SWAP_IS_FREE);
	if (status != FLASH_COMPLETE) {
		return EEPROM_RESULT_FLASH_FAILD;
	}
	return result;
}

uint8_t EEprom::checkCapacity() {
	uint32_t cursor = getLastBlockAddress(startAddress);

	while (cursor >= startAddress) {
		uint16_t blockVirtualAddress = *(__IO uint16_t*) cursor;
		if (blockVirtualAddress != PAGE_IS_FREE) {
			uint8_t counter = 0;
			uint32_t searchCursor = getLastBlockAddress(startAddress);
			while (searchCursor >= startAddress) {
				uint16_t searchVirtualAddress = *(__IO uint16_t*) cursor;
				if (searchVirtualAddress == blockVirtualAddress) {
					counter++;
					if (counter > 1) {
						//find two virtual address in page; need cleanup
						return EEPROM_RESULT_OK;
					}
				}
			}
		}
		cursor = getPrevBlockAddress(cursor);
	}

	return EEPROM_RESULT_FULL;
}

uint8_t EEprom::searchVirtualAddressInSwap(uint32_t virtualAddress) {
	uint32_t cursor = startAddressSwapPage;
	while (cursor < startAddressSwapPage + PAGE_SIZE ) {
		uint16_t vaddressInSwap = *(__IO uint16_t*) cursor;
		if (vaddressInSwap == virtualAddress) {
			return 1;
		}
		if (vaddressInSwap == SWAP_IS_FREE) {
			return 0;
		}
		cursor = getNextBlockAddress(cursor);
	}
	return 0;
}

uint32_t EEprom::getFreeBlockAddress() {
	uint32_t cursor = getLastBlockAddress(startAddress);
	while (cursor >= startAddress) {
		uint16_t blockVirtualAddress = *(__IO uint16_t*) cursor;
		if (blockVirtualAddress != PAGE_IS_FREE) {
			break;
		}
		cursor = getPrevBlockAddress(cursor);
	}
	return getNextBlockAddress(cursor);
}
