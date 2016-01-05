# STM32f10x_EEPROM_CPP
EEPROM software emulator for STM32f10x (with C++).

# Preview

The official implementation of emulated EEPROM has limited to one page of flash memory. It is only 256B (for 1k page size) or 512B (for 2k page). Many applications need more place to storage own data permanently.
This project based on idea described in AN2594 note (http://www.st.com/web/en/catalog/tools/PF257846) , but you can add many pages with different data block size. Your block size depend on purpose.

# Instruction
```C++
	//test eeprom
	EEprom* eeprom1 = new EEprom(5);
	EEprom* eeprom2 = new EEprom(2);

	for (int i = 0; i < 20; i++) {
		uint8_t dataTable[5];
		for (int data = 0; data < 5; data++) {
			dataTable[data] = i + data;
		}
		eeprom1->write(i + 1, dataTable);
	}

	for (int i = 0; i < 40; i++) {
		uint8_t dataTable[2];
		for (int data = 0; data < 2; data++) {
			dataTable[data] = i + data * 2;
		}
		eeprom2->write(i + 1, dataTable);
	}

	int result = 1;
	for (int i = 0; i < 20; i++) {
		uint8_t dataTable[5];
		eeprom1->read(i + 1, dataTable);
		for (int data = 0; data < 5; data++) {
			if (dataTable[data] != i + data) {
				result = 0;
			}
		}
	}
	
	for (int i = 0; i < 40; i++) {
		uint8_t dataTable[2];
		eeprom2->read(i + 1, dataTable);
		for (int data = 0; data < 2; data++) {
			if (dataTable[data] != i + data * 2) {
				result = 0;
			}
		}

	}
	while (!result) {
		;//assert!
	}
```

# Project Status

Almost finished. We have Realase Candidate.

# ...good to know....
(from manual page 60; DocID13902 Rev 15)

Programming and erasing the Flash memory
The Flash memory can be programmed 16 bits (half words) at a time.
For write and erase operations on the Flash memory (write/erase), the internal RC oscillator
(HSI) must be ON.
The Flash memory erase operation can be performed at page level or on the whole Flash
area (mass-erase). The mass-erase does not affect the information blocks.
To ensure that there is no over-programming, the Flash Programming and Erase Controller
blocks are clocked by a fixed clock.
The End of write operation (programming or erasing) can trigger an interrupt. This interrupt
can be used to exit from WFI mode, only if the FLITF clock is enabled. Otherwise, the
interrupt is served only after an exit from WFI.
The FLASH_ACR register is used to enable/disable prefetch and half cycle access, and to
control the Flash memory access time according to the CPU frequency. The tables below
provide the bit map and bit descriptions for this register.
For complete information on Flash memory operations and register configurations, please
refer to the STM32F10xxx Flash programming manual (PM0075) or to the XL
STM32F10xxx Flash programming manual (PM0068).

# Credit

Radomir Mazoń
