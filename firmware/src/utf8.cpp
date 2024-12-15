#include "utf8.h"

//increment = 1 -> increment offset
uint32_t deserialize_utf8(const uint8_t *data, uint16_t *offset, uint8_t increment)
{
    data += *offset;


    if (data[0] == 0)
	return 0;

    if (data[0] < 128)
    {
	//one byte sequence
	if (increment)
	    *offset += 1;
	return (uint32_t)data[0];
    }
    else if ((data[0] >= 194) && (data[0] <= 223))
    {
	if ((data[1] >= 128) && (data[1] <= 191))
	{
	    //two byte sequence
	    if (increment)
		*offset += 2;
	    return (((uint32_t)(data[0]&0x1F))<<6) | ((uint32_t)data[1]&0x3F);
	}
    }
    else if ((data[0] >= 224) && (data[0] <= 239))
    {
	if ((data[1] >= 128) && (data[1] <= 191))
	{
	    if ((data[2] >= 128) && (data[2] <= 191))
	    {
		//three byte sequence
		if (increment)
		    *offset += 3;
		return (((uint32_t)(data[0]&0x0F))<<12) | (((uint32_t)(data[1]&0x3F))<<6) | ((uint32_t)data[2]&0x3F);
	    }
	}
    }
    else if ((data[0] >= 240) && (data[0] <= 244))
    {
	if ((data[1] >= 128) && (data[1] <= 191))
	{
	    if ((data[2] >= 128) && (data[2] <= 191))
	    {
		if ((data[3] >= 128) && (data[3] <= 191))
		{
		    if (increment)
			*offset += 4;
		    return (((uint32_t)(data[0]&0x07))<<18) | (((uint32_t)(data[1]&0x3F))<<12) | (((uint32_t)(data[2]&0x3F))<<6) | ((uint32_t)data[3]&0x3F);
		}

	    }
	}
    }
    else
    {
	//error
	return 0;
    }

}


uint8_t get_next_utf8_offset(const uint8_t *data)
{
    if (data[0] < 128)
    {
	//one byte sequence
	return 1;
    }
    else if ((data[0] >= 194) && (data[0] <= 223))
    {
	if ((data[1] >= 128) && (data[1] <= 191))
	{
	    //two byte sequence
	    return 2;
	}
    }
    else if ((data[0] >= 224) && (data[0] <= 239))
    {
	if ((data[1] >= 128) && (data[1] <= 191))
	{
	    if ((data[2] >= 128) && (data[2] <= 191))
	    {
		//three byte sequence
		return 3;
	    }
	}
    }
    else if ((data[0] >= 240) && (data[0] <= 244))
    {
	if ((data[1] >= 128) && (data[1] <= 191))
	{
	    if ((data[2] >= 128) && (data[2] <= 191))
	    {
		if ((data[3] >= 128) && (data[3] <= 191))
		{
		    return 4;
		}

	    }
	}
    }
    else
    {
	//error
	return 0;
    }

}
