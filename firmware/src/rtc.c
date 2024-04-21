#include "rtc.h"
#include "main.h"
#include "hardware.h"


uint8_t init_rtc(void)
{

    //init I2C -> main.c
    //


    return 1;
}


uint8_t bcd_to_uint8(uint8_t bcd)
{
    uint8_t val = 0;

    val = bcd & 0x0F;
    val += ((bcd & 0xF0)>>4) * 10;

    return val;
}

uint8_t uint8_to_bcd(uint8_t val)
{
    uint8_t bcd = 0;
    if (val > 99)
	return 0;

    bcd = val % 10;
    bcd |= (val/10)<<4;

    return bcd;
}

//return unix_timestamp
uint64_t read_rtc(void)
{
    
    uint8_t data[10];
    uint8_t i = 0;
    HAL_StatusTypeDef result;

    result = HAL_I2C_Mem_Read(&hi2c1, RTC_ADDR, 0x03, 1, data, 7, 15); //0x03 -> seconds register

    if (result != HAL_OK)
    {
	//shit
	return 0;
    }

    const uint8_t dayspermonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    uint8_t seconds = bcd_to_uint8(data[0]);
    uint8_t minutes = bcd_to_uint8(data[1]);
    uint8_t hours = bcd_to_uint8(data[2]);
    uint8_t days = bcd_to_uint8(data[3]);
    uint8_t weekday = bcd_to_uint8(data[4]);
    uint8_t months = bcd_to_uint8(data[5]);
    uint8_t years = bcd_to_uint8(data[6]);

    if (seconds & 0x80) //check osc fault bit
    {
	//time needs to be set again
	return 0;
    }

    //calculate unix epoch
    //year is 2000-2099 (sorry timetravelers)
    uint64_t epoch = 946684800; // = 1.1.2000 0:00:00 GMT

    epoch += (uint64_t)years * 365 * 24 * 3600;
    epoch += (((uint64_t)years/4)+1) * 24* 3600; //leap days
    if (months < 3)
    {
	//remove one leap day, because it did not happen yet
	epoch -= 24*3600;
    } 

    //months
    for (i = 0; i<(months-1); i++)
    {
	epoch += (uint64_t)dayspermonth[i] * 24 * 3600;
    }

    //days
    epoch += ((uint64_t)days-1) * 24 * 3600;

    //rest
    epoch +=(uint64_t) seconds + 60* (uint64_t)minutes + 3600*(uint64_t)hours;

    return epoch;

}

__attribute__((used))
uint8_t write_rtc(uint8_t years, uint8_t months, uint8_t weekday, uint8_t days, uint8_t hours, uint8_t minutes, uint8_t seconds)
{

    uint8_t data[10];
    //uint8_t i = 0;
    HAL_StatusTypeDef result;

    //uint8_t seconds = 0;
    //uint8_t minutes = 25;
    //uint8_t hours = 21;
    //uint8_t days = 13;
    //uint8_t weekday = 6;
    //uint8_t months = 4;
    //uint8_t years = 24;

    //epoch -= 946684800; //offset 1.1.2000 0:00
    //if (epoch <0)
//	return 0;

  //  while (epoch > 365)
    //{

    //}



    data[0] = 0x00;//control1
    data[1] = 0x00;//control2
    data[2] = 0xa0;//control3
    data[3] = uint8_to_bcd(seconds);
    data[4] = uint8_to_bcd(minutes);
    data[5] = uint8_to_bcd(hours);
    data[6] = uint8_to_bcd(days);
    data[7] = uint8_to_bcd(weekday);
    data[8] = uint8_to_bcd(months);
    data[9] = uint8_to_bcd(years);

    result = HAL_I2C_Mem_Write(&hi2c1, RTC_ADDR, 0x00, 1, data, 10, 15); //0x01 -> control1 register

    if (result != HAL_OK)
    {
	//shit
	return 0;
    }
    return 1;


}
