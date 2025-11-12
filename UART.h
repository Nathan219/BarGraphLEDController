#ifndef UART_H
#define UART_H

#include <Arduino.h>

// Function declarations
void initUART();
void handleInput(String msg);
void uartTask(void* parameter);

#endif // UART_H

