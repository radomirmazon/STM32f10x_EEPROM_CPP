# STM32f10x_EEPROM_CPP
EEPROM software emulator for STM32f10x (with C++).

# Preview

The official implementation of emulated EEPROM has limited to one page of flash memory. It is only 256B (for 1k page size) or 512B (for 2k page). Many applications need more place to storage own data permanently.
This project based on idea described in AN2594 note (http://www.st.com/web/en/catalog/tools/PF257846) , but you can add many pages with different data block size. Your block size depend on purpose. Minimal block size is 2B, and maximum block size is 30B. 

# Instruction

# List of Features

# Credit

Radomir Mazoń
