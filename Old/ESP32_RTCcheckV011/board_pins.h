// board_pins.h
// Unified pin definitions for ESP32 families and Arduino Nano ESP32
// Shows used pins, safe unused pins, and unsafe pins (documented)
// Makes it easy to keep track of pin usage across boards

#pragma once

// -----------------------------------------------------------------------------
// Manual board selection (uncomment one if needed)
// #define BOARD_ESP32_S3
// #define BOARD_ESP32_C3
// #define BOARD_ESP32_H2
// #define BOARD_ARDUINO_NANO_ESP32

// -----------------------------------------------------------------------------
// Auto-detect
#if !defined(BOARD_ESP32_S3) && !defined(BOARD_ESP32_C3) && \
    !defined(BOARD_ESP32_H2) && !defined(BOARD_ARDUINO_NANO_ESP32)

  #if defined(ARDUINO_ESP32S3_DEV) || defined(CONFIG_IDF_TARGET_ESP32S3)
    #define BOARD_ESP32_S3
  #elif defined(ARDUINO_ESP32C3_DEV) || defined(CONFIG_IDF_TARGET_ESP32C3)
    #define BOARD_ESP32_C3
  #elif defined(ARDUINO_ESP32H2_DEV) || defined(CONFIG_IDF_TARGET_ESP32H2)
    #define BOARD_ESP32_H2
  #elif defined(ARDUINO_NANO_ESP32)
    #define BOARD_ARDUINO_NANO_ESP32
  #else
    #warning "Unknown board: please define BOARD_ESP32_S3 / C3 / H2 / NANO manually"
  #endif
#endif

// -----------------------------------------------------------------------------
// ESP32-S3
// Safe GPIOs: 1–14, 16–21, 33–48
#ifdef BOARD_ESP32_S3

  // ⚠️ Unsafe pins (don’t use!)
  // 0 = strapping (boot), 15 = USB D-, 16/17 = USB D+ (if native USB),
  // 19/20 = XTAL, 46 = strapping
  // Flash/PSRAM pins: 26–32 (not exposed)

  // ➖ Empty safe pins
   #define PIN_BTN       0
  #define EMPTY_01   1
  #define EMPTY_02   2
  #define EMPTY_03   3
  #define EMPTY_04   4
  #define EMPTY_05   5
  #define EMPTY_06   6
  #define EMPTY_07   7
  #define PIN_LED       8
  #define EMPTY_09   9
  #define EMPTY_10   10
  #define EMPTY_11   11
  #define EMPTY_12   12
  #define EMPTY_13   13
  #define EMPTY_14   14
  #define PIN_I2C_SCL   17
  #define PIN_I2C_SDA   18
  #define EMPTY_21   21
  #define EMPTY_33   33
  #define EMPTY_34   34
  #define EMPTY_35   35
  #define EMPTY_36   36
  #define EMPTY_37   37
  #define EMPTY_38   38
  #define EMPTY_39   39
  #define EMPTY_40   40
  #define EMPTY_41   41
  #define EMPTY_42   42
  #define PIN_UART_TX   43
  #define PIN_UART_RX   44
  #define EMPTY_45   45
  #define EMPTY_47   47
  #define EMPTY_48   48
#endif

// -----------------------------------------------------------------------------
// ESP32-C3
// Safe GPIOs: 0–10, 18–21
#ifdef BOARD_ESP32_C3

  // ⚠️ Unsafe pins (don’t use!)
  // 11–17 = SPI flash/PSRAM (internal use)

  // ➖ Empty safe pins
  #define EMPTY_00   0
  #define EMPTY_01   1
  #define EMPTY_02   2
  #define EMPTY_03   3
  #define PIN_I2C_SDA   4
  #define PIN_I2C_SCL   5
  #define EMPTY_06   6
  #define EMPTY_07   7
  #define PIN_LED       8
  #define PIN_BTN       9
  #define EMPTY_10   10
  #define EMPTY_18   18
  #define EMPTY_19   19
  #define PIN_UART_TX   21
  #define PIN_UART_RX   20  
#endif

// -----------------------------------------------------------------------------
// ESP32-H2
// Safe GPIOs: 0–21, 26–27
#ifdef BOARD_ESP32_H2

  // ⚠️ Unsafe pins (don’t use!)
  // 22–25 = radio/oscillator
  // 28+ = N/A on most packages

  // ➖ Empty safe pins
  #define EMPTY_00   0
  #define EMPTY_01   1
  #define EMPTY_02   2
  #define EMPTY_03   3
  #define PIN_I2C_SDA   4
  #define PIN_I2C_SCL   5
  #define EMPTY_06   6
  #define EMPTY_07   7
  #define PIN_LED       8
  #define PIN_BTN       9
  #define EMPTY_10   10
  #define EMPTY_11   11
  #define EMPTY_12   12
  #define EMPTY_13   13
  #define EMPTY_14   14
  #define EMPTY_15   15
  #define EMPTY_16   16
  #define EMPTY_17   17
  #define EMPTY_18   18
  #define EMPTY_19   19
  #define PIN_UART_TX   20
  #define PIN_UART_RX   21
  #define EMPTY_26   26
  #define EMPTY_27   27
#endif

// -----------------------------------------------------------------------------
// Arduino Nano ESP32
// Uses Arduino Dx / Ax macros
#ifdef BOARD_ARDUINO_NANO_ESP32

  // ⚠️ Unsafe pins
  // Depends on Nano ESP32 design; usually USB, flash and RF pins are abstracted away
 
  // ➖ Digital pins

  #define PIN_UART_RX   D0
  #define PIN_UART_TX   D1
  #define EMPTY_D2   D2
  #define EMPTY_D3   D3
  #define EMPTY_D4   D4
  #define EMPTY_D5   D5
  #define EMPTY_D6   D6
  #define EMPTY_D7   D7
  #define EMPTY_D8   D8
  #define PIN_BTN      D9
  #define EMPTY_D10  D10
  #define EMPTY_D11  D11
  #define EMPTY_D12  D12
  #define EMPTY_D13  D13
  #define PIN_LED       LED_BUILTIN

  // ➖ Analog pins
  #define EMPTY_A0   A0
  #define EMPTY_A1   A1
  #define EMPTY_A2   A2
  #define EMPTY_A3   A3
  #define PIN_I2C_SDA   A4
  #define PIN_I2C_SCL   A5
  #define EMPTY_A6   A6
  #define EMPTY_A7   A7
#endif
