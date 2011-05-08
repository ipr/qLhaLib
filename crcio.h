#ifndef CRCIO_H
#define CRCIO_H

#include <stdint.h>
#include <limits.h>

// 16-bit CRC table and counting,
// could make static instance of this..
// CRC table is only modified when instance is created.
//
class CCrcIo
{
protected:
	unsigned int m_crctable[UCHAR_MAX + 1];
	//int      dispflg;

	void make_crctable();

	inline unsigned int UPDATE_CRC(unsigned int crc, unsigned int c)
	{
		return m_crctable[((crc) ^ (c)) & 0xFF] ^ ((crc) >> CHAR_BIT);
	}
	
public:
    CCrcIo()
	{
		make_crctable();
	};
	
	unsigned int UpdateCrc(unsigned int crc, unsigned int c)
	{
		return UPDATE_CRC(crc, c);
	}
	
	unsigned int calccrc(unsigned int crc, unsigned char *p, unsigned int n);
};

#endif // CRCIO_H
