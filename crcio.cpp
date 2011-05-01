#include "crcio.h"

#define CRCPOLY         0xA001      /* CRC-16 (x^16+x^15+x^2+1) */


void CCrcIo::make_crctable()
{
    unsigned int    i, j, r;

    for (i = 0; i <= UCHAR_MAX; i++) 
	{
        r = i;
        for (j = 0; j < CHAR_BIT; j++)
		{
            if (r & 1)
			{
                r = (r >> 1) ^ CRCPOLY;
			}
            else
			{
                r >>= 1;
			}
		}
        m_crctable[i] = r;
    }
}

unsigned int CCrcIo::calccrc(unsigned int crc, unsigned char *p, unsigned int n)
{
    while (n-- > 0)
	{
        crc = UPDATE_CRC(crc, *p++);
	}
    return crc;
}

