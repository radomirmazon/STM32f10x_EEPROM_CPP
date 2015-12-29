# STM32f10x_EEPROM_CPP
EEPROM software emulator for STM32f10x (with C++).

# Preview

The official implementation of emulated EEPROM has limited to one page of flash memory. It is only 256B (for 1k page size) or 512B (for 2k page). Many applications need more place to storage own data permanently.
This project based on idea described in AN2594 note (http://www.st.com/web/en/catalog/tools/PF257846) , but you can add many pages with different data block size. Your block size depend on purpose. Minimal block size is 2B, and maximum block size is 30B. 

# Instruction
```C++
uint8_t data[8];
uint8_t data2[8];

//init data:
uint8_t temp = 11;
for (int i=0; i<8; i++) {
	data[i] = temp;
	temp += 11;
}


EEprom* eeprom = new EEprom(8);
eeprom->write(0x0001, data)
eeprom->read(0x0001, data2);

//test
for (int i=0; i<8; i++) {
	if (data[i] != data2[i]) {
		assert(); //!!	
	}
} 
```

# List of Features

# Credit

Radomir Mazoń
