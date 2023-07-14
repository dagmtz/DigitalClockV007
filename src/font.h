/*
 *  font.h
 *  i2c
 *
 *  Created by Michael Köhler on 13.09.18.
 *  Copyright 2018 Skie-Systems. All rights reserved.
 *
 */
#ifndef _FONT_H_
#define _FONT_H_

#include <avr/pgmspace.h>

extern const char ssd1306oled_font[][6] PROGMEM;
extern const char special_char[][2] PROGMEM;

#endif /* _FONT_H_ */
