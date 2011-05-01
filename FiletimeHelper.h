/////////////////////////////////////
//
// Windows FILETIME-timestamp conversion helper
//
// Ilkka Prusi 2011
//

#ifndef _FILETIMEHELPER_H_
#define _FILETIMEHELPER_H_

#include <stdint.h>

// for typedefs used here
#include <Windows.h>


class CFiletimeHelper
{
private:
	uint64_t m_u64Stamp;
	
protected:
	uint64_t GetFiletimeValue(const FILETIME &ftStamp) const
	{
	//ifdef _WIN64
		// 64-bit -> need helper for correct alignment
	//else
		// 32-bit -> can use faster casting-way
	//endif
		
		// for now, just use same method for both.. optimize later
		ULARGE_INTEGER ulTemp;
		ulTemp.HighPart = ftStamp.dwHighDateTime;
		ulTemp.LowPart = ftStamp.dwLowDateTime;
		return ulTemp.QuadPart;
	}
	
	FILETIME GetValueAsFiletime(const uint64_t u64Stamp) const
	{
		FILETIME ftStamp;
		ULARGE_INTEGER ulTemp;
		ulTemp.QuadPart = u64Stamp;
		ftStamp.dwHighDateTime = ulTemp.HighPart;
		ftStamp.dwLowDateTime = ulTemp.LowPart;
		return ftStamp;
	}
	
public:
    CFiletimeHelper()
		: m_u64Stamp(0)
	{}
    CFiletimeHelper(const uint64_t u64Stamp)
		: m_u64Stamp(u64Stamp)
	{}
    CFiletimeHelper(const FILETIME &ftStamp)
		: m_u64Stamp(0)
	{
		m_u64Stamp = GetFiletimeValue(ftStamp);
	}
    CFiletimeHelper(const unsigned long ulHiPart, const unsigned long ulLoPart)
		: m_u64Stamp(0)
	{
		FILETIME ft;
		ft.dwHighDateTime = ulHiPart;
		ft.dwLowDateTime = ulLoPart;
		m_u64Stamp = GetFiletimeValue(ft);
	}
    ~CFiletimeHelper()
	{}
	
	operator uint64_t() const
	{
		return m_u64Stamp;
	}

	operator FILETIME() const
	{
		return GetValueAsFiletime(m_u64Stamp);
	}

	bool operator ==(const CFiletimeHelper &Helper) const
	{
		if (this == &Helper)
		{
			return true;
		}
		return (m_u64Stamp == (uint64_t)Helper);
	}
	
	bool operator !=(const CFiletimeHelper &Helper) const
	{
		return (!this->operator ==(Helper));
	}

	/*
	bool operator ==(const FILETIME &ftStamp) const
	{
		return (m_u64Stamp == GetFiletimeValue(ftStamp));
	}
	
	bool operator !=(const FILETIME &ftStamp) const
	{
		return (!this->operator ==(ftStamp));
	}
	*/
	
	CFiletimeHelper& operator =(const FILETIME &ftStamp)
	{
		m_u64Stamp = GetFiletimeValue(ftStamp);
		return *this;
	}

	CFiletimeHelper& operator =(const CFiletimeHelper &Helper)
	{
		// check self-assignment
		if (this == &Helper)
		{
			return *this;
		}
		
		m_u64Stamp = (uint64_t)Helper;
		return *this;
	}

	uint64_t operator +(const CFiletimeHelper &Helper) const
	{
		return (m_u64Stamp + (uint64_t)Helper);
	}
	
	uint64_t operator -(const CFiletimeHelper &Helper) const
	{
		return (m_u64Stamp - (uint64_t)Helper);
	}
	
	uint64_t GetValue() const
	{
		return m_u64Stamp;
	}

	uint64_t GetAsUnixTime() const
	{
		// 1. epoch-conversion
		// 2. hectonanoseconds to seconds
		return ((m_u64Stamp - 116444736000000000ULL) / 10000000ULL);
	}
	
	CFiletimeHelper& SetNow()
	{
		FILETIME ftNow;
		::GetSystemTimeAsFileTime(&ftNow);
		
		m_u64Stamp = GetFiletimeValue(ftNow);
		return *this;
	}
	
	/*
	wstring ToString()
	{}
	*/
	
};

#endif // _FILETIMEHELPER_H_


