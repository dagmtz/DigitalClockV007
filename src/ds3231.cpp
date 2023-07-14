
#include <util/delay.h>

#include "ds3231.h"

static uint8_t addressPtr = 0x00;

void addressInc()
{
  addressPtr++;
  if ( addressPtr >= 0x12 )
  {
    addressPtr = 0x00;
  }
}


void DS3231_getAll(DS3231_buffer_t *p_buffer)
{
  if(addressPtr != 0x00)
  {
    addressPtr = 0x00;
    /* Set DS3231 Address Pointer to the desired address */
    i2c_start_sla(TW_SLA_W(DS3231_ADDRESS));
    i2c_write(addressPtr);
    i2c_stop();
  }
  
  i2c_start_sla(TW_SLA_R(DS3231_ADDRESS));
  
  p_buffer->seconds.byte = i2c_readAck();
  p_buffer->minutes.byte = i2c_readAck();
  p_buffer->hours.byte = i2c_readAck();
  p_buffer->days.byte = i2c_readAck();
  p_buffer->date.byte = i2c_readAck();
  p_buffer->month_century.byte = i2c_readAck();
  p_buffer->years.byte = i2c_readAck();
  p_buffer->a1seconds.byte = i2c_readAck();
  p_buffer->a1minutes.byte = i2c_readAck();
  p_buffer->a1hour.byte = i2c_readAck();
  p_buffer->a1day.byte = i2c_readAck();
  p_buffer->a2minutes.byte = i2c_readAck();
  p_buffer->a2hour.byte = i2c_readAck();
  p_buffer->a2day.byte = i2c_readAck();
  p_buffer->control.byte = i2c_readAck();
  p_buffer->control_status.byte = i2c_readAck();
  p_buffer->aging.byte = i2c_readAck();
  p_buffer->temp_msb.byte = i2c_readAck();
  p_buffer->temp_lsb.byte = i2c_readNAck();
  
  i2c_stop();  
}

void DS3231_setByte(uint8_t byteToSet, uint8_t value)
{
  
  /* Set DS3231 Address Pointer to the desired address */
  i2c_start_sla(TW_SLA_W(DS3231_ADDRESS));
  i2c_write(byteToSet);
  
  /* Set the desired value to the address set */
  i2c_write(value);
  i2c_stop();
  
  addressInc();
}

bool DS3231_getAMPM()
{
  bitfield8_t buffer = {0};
  buffer.byte = DS3231_getByte(DS3231_HOURS);
  
  /* 0 = 24h, 1 = 12h */
  return buffer.bits.b6;
}

bool DS3231_getCentury()
{
  bitfield8_t buffer = {0};
  buffer.byte = DS3231_getByte(DS3231_MONTH);
  
  /* 0 = XXI, 1 = XXII */
  return buffer.bits.b7;
}

uint8_t DS3231_getByte(uint8_t byteToGet)
{
  uint8_t buffer = 0;
  
  if(addressPtr != byteToGet)
  {
    addressPtr = byteToGet;
    /* Set DS3231 Address Pointer to the desired address */
    i2c_start_sla(TW_SLA_W(DS3231_ADDRESS));
    i2c_write(byteToGet);
    i2c_stop();
  }
  else
  {
    /* Request one byte and send NACK to end transmission */
    i2c_start_sla(TW_SLA_R(DS3231_ADDRESS));
    buffer = i2c_readNAck();
    i2c_stop();
  }
  
  addressInc();
  return buffer;
}

uint8_t DS3231_getMonth()
{
  bitfield8_t buffer = {0};
  buffer.byte = DS3231_getByte(DS3231_MONTH);
  buffer.bits.b7 = 0;
  
  return buffer.byte;
}

float DS3231_getTemp()
{

  uint8_t tMSB, tLSB;
  float temp;
  
  if(addressPtr != DS3231_TEMP_MSB)
  {
    addressPtr = DS3231_TEMP_MSB;
    /* Set DS3231 Address Pointer to the desired address */
    i2c_start_sla(TW_SLA_W(DS3231_ADDRESS));
    i2c_write(DS3231_TEMP_MSB);
    i2c_stop();
  }
  
  i2c_start_sla(TW_SLA_R(DS3231_ADDRESS));
  tMSB = i2c_readAck();
  addressInc();
  tLSB = i2c_readNAck();
  addressInc();
  i2c_stop();
  
  int16_t _temp = ( tMSB << 8 | (tLSB & 0xC0) );  // Shift upper byte, add lower
  temp = ( (float) _temp / 256.0 );              // Scale and return
  
  return temp;
}
