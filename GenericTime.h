#ifndef GENERICTIME_H
#define GENERICTIME_H

/*
 * Generic (MS-DOS style) time stamp format (localtime):
 *
 *  31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16
 * |<---- year-1980 --->|<- month ->|<--- day ---->|
 *
 *  15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 * |<--- hour --->|<---- minute --->|<- second/2 ->|
 *
 */

#include <stdint.h>
#include <time.h>


// some kinda shitty msdos-timestamp.. blah..
// just convert it
//
class CGenericTime
{
protected:
	long m_lGenericTime;
	
	// inline conversion helpers

	inline int subbits(int n, int off, int len) const
	{
		return (((n) >> (off)) & ((1 << (len))-1));
	}

	inline time_t generic_to_unix_stamp(const long t) const
	{
		struct tm tm;
	
		tm.tm_sec  = subbits(t,  0, 5) * 2;
		tm.tm_min  = subbits(t,  5, 6);
		tm.tm_hour = subbits(t, 11, 5);
		tm.tm_mday = subbits(t, 16, 5);
		tm.tm_mon  = subbits(t, 21, 4) - 1;
		tm.tm_year = subbits(t, 25, 7) + 80;
		tm.tm_isdst = -1;
	
		return mktime(&tm);
	}

	/*
	inline long unix_to_generic_stamp(const time_t t) const
	{
		struct tm *tm = localtime(&t);
	
		tm->tm_year -= 80;
		tm->tm_mon += 1;
	
		return ((long)(tm->tm_year << 25) +
				(tm->tm_mon  << 21) +
				(tm->tm_mday << 16) +
				(tm->tm_hour << 11) +
				(tm->tm_min  << 5) +
				(tm->tm_sec / 2));
	}
	*/
	
	
public:
	CGenericTime(void)
		: m_lGenericTime(0)
	{}
	CGenericTime(const long lTime)
		: m_lGenericTime(lTime)
	{}
	
	operator time_t () const
	{
		return generic_to_unix_stamp(m_lGenericTime);
	}

};

#endif // GENERICTIME_H
