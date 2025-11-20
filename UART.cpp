#include "UART.h"
#include "CommandParser.h"

// UART hardware
#define UART_RX 8
#define UART_TX 7
#define UART_BAUD 38400
HardwareSerial extSerial(1);

void initUART() {
  extSerial.begin(UART_BAUD, SERIAL_8N1, UART_RX, UART_TX);
}

void uartTask(void* parameter){
  while(true){
    if (Serial.available()) {
      String line = Serial.readStringUntil('\n');
      if (line.length() > 0 && line.length() < 200) {  // Safety check: limit line length
        handleInput(line);
      }
    }
    if (extSerial.available()) {
      String line = extSerial.readStringUntil('\n');
      if (line.length() > 0 && line.length() < 200) {  // Safety check: limit line length
        handleInput(line);
      }
    }
    // Give the idle task a chance to run and reset the watchdog
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

