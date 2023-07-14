#include "clock.h"

uint8_t tickSeconds(clock_control_t *clockControl)
{
    clockControl->time.seconds++;

    if (clockControl->time.seconds >= SECONDS_PER_MINUTE)
    {
        clockControl->time.seconds = 0;
    }

    return clockControl->time.seconds;
}

uint8_t div10(uint8_t number)
{
    uint8_t q, r;
    q = (number >> 1) + (number >> 2);
    q = q + (q >> 4);
    q = q + (q >> 8);
    q = q >> 3;
    r = number - (((q << 2) + q) << 1);
    return q + (r > 9);
}

uint8_t bcdToDec(uint8_t value)
{
    /* Convert binary coded decimal to normal decimal numbers */
    return ((value / 16 * 10) + (value % 16));
}

uint8_t getWeekday(const date_ymd_t *date)
{
    uint16_t y = date->years.yyyy;
    uint8_t m = date->months;
    uint8_t d = date->days;

    return (d += m < 3 ? y-- : y - 2, 23 * m / 9 + d + 4 + y / 4 - y / 100 + y / 400) % 7;
}

void printTime(clock_control_t *clockControl)
{
    char buffer[40];
    // sprintf(buffer, "%04d-%02d-%02d\t%02d:%02d:%02d\n", clockControl->date.years.yyyy, clockControl->date.months, clockControl->date.days, clockControl->time.hours, clockControl->time.minutes, clockControl->time.seconds);
    // uart_puts(buffer);
    sprintf(buffer, "%02d:%02d:%02d\n", clockControl->time.hours, clockControl->time.minutes, clockControl->time.seconds);
    uart_puts(buffer);
}

void timeToBCD(const time_hms_t timeBuffer, uint8_t *bcdBuffer)
{
    bcdBuffer[0] = div10(timeBuffer.hours);
    bcdBuffer[1] = timeBuffer.hours - (bcdBuffer[0] * 10);
    bcdBuffer[2] = div10(timeBuffer.minutes);
    bcdBuffer[3] = timeBuffer.minutes - (bcdBuffer[2] * 10);
    bcdBuffer[4] = div10(timeBuffer.seconds);
    bcdBuffer[5] = timeBuffer.seconds - (bcdBuffer[4] * 10);
}

void clockToLED(uint8_t *buffer)
{
    i2c_start_sla(TW_SLA_W(LED_DRIVER_ADDRESS));

    for (size_t i = 0; i < 7; i++)
    {
        i2c_write(buffer[i]);
    }

    i2c_stop();
}

void clockToOLED( clock_control_t *clockControl )
{
    char buffer[12];
    
    oled_home();

    weekdayToString(clockControl->weekday, buffer);
    oled_puts(buffer);

    oled_gotoxy(0, 3);
    sprintf(buffer, "%02d-%02d-%04d", clockControl->date.days, clockControl->date.months, clockControl->date.years.yyyy);
    oled_puts(buffer);

    oled_gotoxy(0, 6);
    sprintf(buffer, "%.2f°C", clockControl->temperature);
    oled_puts(buffer);
}

void weekdayToString(uint8_t weekday, char *string)
{
    switch (weekday)
    {
    case SUNDAY:
        sprintf(string, "Sunday");
        break;

    case MONDAY:
        sprintf(string, "Monday");
        break;

    case TUESDAY:
        sprintf(string, "Tuesday");
        break;

    case WEDNESDAY:
        sprintf(string, "Wednesday");
        break;

    case THURSDAY:
        sprintf(string, "Thursday");
        break;

    case FRIDAY:
        sprintf(string, "Friday");
        break;

    case SATURDAY:
        sprintf(string, "Saturday");
        break;

    default:
        break;
    }
}

void syncControl(uint8_t address, DS3231_buffer_t *buffer, clock_control_t *control)
{
    switch (address)
    {
    case DS3231_SECONDS:
        control->time.seconds = ((buffer->seconds.nibbles.ms_nibble * 5) << 1) + buffer->seconds.nibbles.ls_nibble;
        break;

    case DS3231_MINUTES:
        control->time.minutes = ((buffer->minutes.nibbles.ms_nibble * 5) << 1) + buffer->minutes.nibbles.ls_nibble;
        break;

    case DS3231_HOURS:
        if (buffer->hours.bits.b6)
        {
            control->time.hours = ((buffer->hours.bits.b4 * 5) << 1) + buffer->hours.nibbles.ls_nibble;
        }
        else
        {
            control->time.hours = ((buffer->hours.nibbles.ms_nibble * 5) << 1) + buffer->hours.nibbles.ls_nibble;
        }
        break;

    case DS3231_DAYS:
        control->weekday = buffer->days.byte;
        break;

    case DS3231_DATE:
        control->date.days = ((buffer->date.nibbles.ms_nibble * 5) << 1) + buffer->date.nibbles.ls_nibble;
        break;

    case DS3231_MONTH:
        control->date.months = ((buffer->month_century.bits.b4 * 5) << 1) + buffer->month_century.nibbles.ls_nibble;
        break;

    case DS3231_YEAR:
        control->date.years.yyyy = 2000U + ((buffer->years.nibbles.ms_nibble * 5) << 1) + buffer->years.nibbles.ls_nibble;
        break;

    default:
        break;
    }
}

void update_clock(clock_control_t *clockControl, const clock_units_t unit, const sign_t sign, const bool affectNextUnit, const bool printTime)
{
    switch (unit)
    {
    case SECONDS:
        switch (sign)
        {
        case MINUS:
            clockControl->time.seconds--;
            if (clockControl->time.seconds >= SECONDS_PER_MINUTE)
            {
                clockControl->time.seconds = SECONDS_PER_MINUTE - 1;
            }
            break;

        case PLUS:
            clockControl->time.seconds++;
            if (clockControl->time.seconds >= SECONDS_PER_MINUTE)
            {
                clockControl->time.seconds = 0;
                if (affectNextUnit)
                {
                    update_clock(clockControl, MINUTES, PLUS, true, false);
                }
            }
            break;

        default:
            break;
        }
        break;

    case MINUTES:
        switch (sign)
        {
        case MINUS:
            clockControl->time.minutes--;
            if (clockControl->time.minutes >= MINUTES_PER_HOUR)
            {
                clockControl->time.minutes = MINUTES_PER_HOUR - 1;
            }
            break;

        case PLUS:
            clockControl->time.minutes++;
            if (clockControl->time.minutes >= MINUTES_PER_HOUR)
            {
                clockControl->time.minutes = 0;
                if (affectNextUnit)
                {
                    update_clock(clockControl, HOURS, PLUS, true, false);
                }
            }
            break;

        default:
            break;
        }
        break;

    case HOURS:
        switch (sign)
        {
        case MINUS:
            clockControl->time.hours--;
            if (clockControl->time.hours >= HOURS_PER_DAY)
            {
                clockControl->time.hours = HOURS_PER_DAY - 1;
            }
            break;

        case PLUS:
            clockControl->time.hours++;
            if (clockControl->time.hours >= HOURS_PER_DAY)
            {
                clockControl->time.hours = 0;
                if (affectNextUnit)
                {
                    update_clock(clockControl, DAYS, PLUS, true, false);
                }
            }
            break;

        default:
            break;
        }
        break;

    case DAYS:
        switch (sign)
        {
        case MINUS:
            clockControl->date.days--;
            if ((clockControl->date.days == 0) || (clockControl->date.days >= clockControl->daysInCurrentMonth))
            {
                clockControl->date.days = month_length(clockControl->date.years.yyyy, clockControl->date.months);
            }
            break;

        case PLUS:
            clockControl->date.days++;
            if (clockControl->date.days >= clockControl->daysInCurrentMonth)
            {
                clockControl->date.days = 1;
                if (affectNextUnit)
                {
                    update_clock(clockControl, MONTHS, PLUS, true, false);
                }
            }
            break;

        case NO_SIGN:
            if (clockControl->date.days >= clockControl->daysInCurrentMonth)
            {
                clockControl->date.days = month_length(clockControl->date.years.yyyy, clockControl->date.months);
            }
            break;

        default:
            break;
        }
        clockControl->weekday = getWeekday(&clockControl->date);
        break;

    case MONTHS:
        switch (sign)
        {
        case MINUS:
            clockControl->date.months--;
            if ((clockControl->date.months == 0) || (clockControl->date.months >= MONTHS_PER_YEAR))
            {
                clockControl->date.months = MONTHS_PER_YEAR;
            }
            clockControl->daysInCurrentMonth = month_length(clockControl->date.months, clockControl->date.years.yyyy);
            update_clock(clockControl, DAYS, NO_SIGN, false, false);
            break;

        case PLUS:
            clockControl->date.months++;
            if (clockControl->date.months >= MONTHS_PER_YEAR)
            {
                clockControl->date.months = 1;
                if (affectNextUnit)
                {
                    update_clock(clockControl, YEARS, PLUS, true, false);
                }
            }
            clockControl->daysInCurrentMonth = month_length(clockControl->date.months, clockControl->date.years.yyyy);
            update_clock(clockControl, DAYS, NO_SIGN, false, false);
            break;

        default:
            break;
        }
        break;

    case YEARS:
        switch (sign)
        {
        case MINUS:
            clockControl->date.years.yyyy--;
            if ((clockControl->date.years.yyyy == 0) || (clockControl->date.years.yyyy >= UPPER_TOP_YEARS))
            {
                clockControl->date.years.yyyy = 0;
            }
            break;

        case PLUS:
            clockControl->date.years.yyyy++;
            if (clockControl->date.years.yyyy >= UPPER_TOP_YEARS)
            {
                clockControl->date.years.yyyy = UPPER_TOP_YEARS;
            }
            break;

        default:
            break;
        }
        clockControl->date.years.__yy = clockControl->date.years.yyyy % 100;
        clockControl->date.years.yy__ = clockControl->date.years.yyyy - clockControl->date.years.__yy;
        break;

    default:
        break;
    }
}
