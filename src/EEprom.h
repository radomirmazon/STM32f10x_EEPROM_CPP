/*
 * Error.h
 *
 *  Created on: 22 lis 2015
 *      Author: radomir
 */

#ifndef ERROR_H_
#define ERROR_H_

#include <stdint.h>

#define ERROR_CAN_SCE 0
#define ERROR_INPUT_BUFFOR_OVERFLOW 1
#define ERROR_CAN_ESR 2
#define ERROR_FATAL 3

#define ERROR_SIZE 6

typedef struct {
	uint8_t code;
	uint8_t count;
} ERROR_t;

class Error {
public:
	void error(uint8_t code);
	void crearErrors();
	ERROR_t getError(uint8_t index);

private:
	ERROR_t errorTab[ERROR_SIZE];
	uint8_t index = 0;
};

#endif /* ERROR_H_ */
