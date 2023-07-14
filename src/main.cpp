#include "clock.h"

static DS3231_buffer_t rtcBuffer;
DS3231_buffer_t *p_rtcBuffer = &rtcBuffer;
static clock_control_t clockControl = {STOP, {0, 0, 0}, {{2000}, 1, 1}, SATURDAY, DAYS_PER_MONTH_MAX, "00:00:00", "2000-01-01", 25.0};
clock_control_t *p_clockCtrl = &clockControl;
static settings_control_t settingsControl = {0, 0, 0, NOT_PRESSED, NOT_PRESSED, NO_UNIT, SECONDS, 0};
settings_control_t *p_settingsCtrl = &settingsControl;

volatile bool g_heartbeat_1s = false;
/* Increases every second (from RTC) */
volatile static uint32_t g_secondsCounter = 0;
/* Increases every 1/1024 of a second, close to 1ms */
volatile static uint32_t g_msCounter = 0;

char g_stringBuffer[30] = "I'm alive\n";
uint8_t g_ledBuffer[7] = {0, 0, 0, 0, 0, 0, 4};

int main(void)
{
  if (init() == 1)
  {
    while (1)
    {
#ifdef _USART_DEBUG
      uart_puts_P("Init failed.\n");
      _delay_ms(5000);
#endif /* _USART_DEBUG */
    }
  }

#ifdef _USART_DEBUG
  uart_puts_P("Clock running\n");
#endif /* _USART_DEBUG */
  oled_clrscr();

  while (1)
  {
    // When a 1hz interrupt is triggered
    if (g_heartbeat_1s)
    {
      // Release and prepare next second trigger
      g_heartbeat_1s = false;

      // First time sync with RTC
      if (p_clockCtrl->clockState == STANDBY)
      {
        DS3231_getAll(p_rtcBuffer);

        syncControl(DS3231_SECONDS, p_rtcBuffer, p_clockCtrl);
        syncControl(DS3231_MINUTES, p_rtcBuffer, p_clockCtrl);
        syncControl(DS3231_HOURS, p_rtcBuffer, p_clockCtrl);
        syncControl(DS3231_DATE, p_rtcBuffer, p_clockCtrl);
        syncControl(DS3231_MONTH, p_rtcBuffer, p_clockCtrl);
        syncControl(DS3231_YEAR, p_rtcBuffer, p_clockCtrl);
        p_clockCtrl->weekday = getWeekday(&p_clockCtrl->date);
        p_clockCtrl->temperature = DS3231_getTemp();
        p_clockCtrl->clockState = RUNNING; 

      }

      // Sync up every minute (?)
      else if (p_clockCtrl->time.seconds == 0)
      {
        p_clockCtrl->temperature = DS3231_getTemp();
        update_clock(p_clockCtrl, MINUTES, PLUS, true, false);
      }

      DS3231_setByte(DS3231_MONTH, 0x07);
      DS3231_setByte(DS3231_DATE, 0x10);
      timeToBCD(p_clockCtrl->time, g_ledBuffer);
      if (p_clockCtrl->clockState == RUNNING)
      {
        clockToLED(g_ledBuffer);
        clockToOLED(p_clockCtrl);
      }
      sprintf(g_stringBuffer, "%04d-%02d-%02d\n%02d:%02d:%02d\n%.1f°C\n", p_clockCtrl->date.years.yyyy, p_clockCtrl->date.months, p_clockCtrl->date.days, p_clockCtrl->time.hours, p_clockCtrl->time.minutes, p_clockCtrl->time.seconds, p_clockCtrl->temperature);
      uart_puts(g_stringBuffer);

#ifdef _USART_DEBUG
      // printTime();
#endif /* IFDEF _USART_DEBUG */
    }
  }
}

uint8_t init()
{
  /* Set up external interrupt (1Hz from RTC) */
  DDRD &= _BV(PORTD2);
  EICRA = (_BV(ISC01) | _BV(ISC00));

  /* Enable INT0 external interrupt */
  EIMSK = _BV(INT0);

#ifdef _USART_DEBUG
  uart_init(UART_BAUD_SELECT(UART_BAUD_RATE, F_CPU));
#endif /* _USART_DEBUG */

  i2c_init();
  oled_init(LCD_DISP_ON); // init lcd and turn on
  oled_puts_p(PSTR("Initializing..."));

  /* Try to check RTC is responding by setting Control Register to 0x00 */
  p_rtcBuffer->control.byte = 0b00011100;
  uint16_t _rtc_tryCounter = 0;
  while (p_rtcBuffer->control.byte != 0)
  {
    if (_rtc_tryCounter >= 50)
    {
#ifdef _USART_DEBUG
      uart_puts_P("Init failed, no RTC response\n");
#endif /* _USART_DEBUG */
      return 1;
    }

    _delay_ms(100);
    DS3231_setByte(DS3231_CONTROL, 0x00);
    p_rtcBuffer->control.byte = DS3231_getByte(DS3231_CONTROL);
    _rtc_tryCounter++;
  }

  sei();

#ifdef _USART_DEBUG
  uart_puts_P("Initialization complete\n");
#endif /* _USART_DEBUG */

  oled_clrscr();
  oled_puts_p(PSTR("Init complete"));

  p_clockCtrl->clockState = STANDBY;

  return 0;
}

ISR(INT0_vect)
{
  g_heartbeat_1s = true;
  g_secondsCounter++;
  if (p_clockCtrl->clockState == RUNNING)
  {
    tickSeconds(p_clockCtrl);
  }
}
