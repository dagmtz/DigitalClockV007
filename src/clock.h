#ifndef CLOCK_H_
#define CLOCK_H_

#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/delay.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include "usart.h"
#include "i2c.h"
#include "oled.h"
#include "ds3231.h"

#define SECONDS_PER_MINUTE 60
#define MINUTES_PER_HOUR 60
#define HOURS_PER_DAY 24
#define DAYS_PER_MONTH_MAX 31
#define MONTHS_PER_YEAR 12
#define UPPER_TOP_YEARS 2999

#define LED_DRIVER_ADDRESS 0x08

#define USART_DEBUG 1

#if defined(USART_H_) && defined(USART_DEBUG)
#define _USART_DEBUG
#else
#undef _USART_DEBUG
#endif /* defined (USART_H_) && defined (USART_DEBUG) */

#define UART_BAUD_RATE 19200

typedef enum
{
  MINUS,
  NO_SIGN,
  PLUS
} sign_t;

typedef enum
{
  SECONDS,
  MINUTES,
  HOURS,
  DAYS,
  DATE,
  MONTHS,
  YEARS,
  NO_UNIT = 0xFF
} clock_units_t;

typedef enum
{
  STOP,
  STANDBY,
  RUNNING,
  SETUP
} clock_state_t;

typedef enum
{
    NOT_PRESSED,
    DEBOUNCE,
    SINGLE,
    HOLD,
    WAIT
} button_press_state_t;

typedef struct
{
  uint16_t yyyy;
  struct
  {
    uint8_t yy__;
    uint8_t __yy;
  };
} years_t;

typedef struct
{
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
} time_hms_t;

typedef struct
{
  years_t years;
  uint8_t months;
  uint8_t days;
} date_ymd_t;

typedef struct
{
  clock_state_t clockState;
  time_hms_t time;
  date_ymd_t date;
  uint8_t weekday;
  uint8_t daysInCurrentMonth;
  char timeString[9];
  char dateString[11];
  float temperature;
} clock_control_t;

typedef struct
{
    uint8_t currentButtonReading;
    uint8_t lastButtonReading;
    uint8_t currentButtonSelected;
    button_press_state_t currentButtonState;
    button_press_state_t lastButtonState;
    clock_units_t currentUnit;
    clock_units_t nextUnit;
    unsigned long lastPressTime;
} settings_control_t;

void printTime( void );
void timeToBCD( const time_hms_t timeBuffer, uint8_t *bcdBuffer );
void clockToLED( uint8_t *buffer );
void clockToOLED( clock_control_t *clockControl );
void weekdayToString( uint8_t weekday, char *string );
void syncControl( uint8_t address, DS3231_buffer_t *buffer, clock_control_t *control );
void update_clock( clock_control_t *clockControl, const clock_units_t unit, const sign_t sign, const bool affectNextUnit, const bool printTime );

uint8_t init( void );
uint8_t tickSeconds( clock_control_t *clockControl );
uint8_t div10( uint8_t number );
uint8_t bcdToDec( uint8_t value );
uint8_t getWeekday( const date_ymd_t *date );

#endif /* CLOCK_H_ */