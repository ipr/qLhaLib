//////////////////////////////////
//
// structure and code for handling archive-file header
//
// Ilkka Prusi 2011
//

#ifndef LZHEADER_H
#define LZHEADER_H

#include <string>

#include "AnsiFile.h"
#include "LhaTypeDefs.h"

#include "crcio.h"
#include "GenericTime.h"
#include "FiletimeHelper.h"

//typedef int boolean;            /* TRUE or FALSE */

#define METHOD_TYPE_STORAGE     5

// this is just ridiculous..
//#define FILENAME_LENGTH 1024


typedef struct LzHeader 
{
	// constructor
	LzHeader()
	{
		memset(this, 0, sizeof(LzHeader));
	}
	void init_header()
	{
	}
	
    size_t          header_size;
    int             size_field_length;
    char            method[METHOD_TYPE_STORAGE];
    size_t          packed_size;
    size_t          original_size;
    unsigned char   attribute;
    unsigned char   header_level;
	std::string     name;
	std::string     dirname;
	std::string     realname;
    //char            name[FILENAME_LENGTH];
    //char            realname[FILENAME_LENGTH];/* real name for symbolic link */
    unsigned int    crc;      /* file CRC */
    bool            has_crc;  /* file CRC */
    unsigned int    header_crc; /* header CRC */
    unsigned char   extend_type;
    unsigned char   minor_version;

    /* extend_type == EXTEND_UNIX  and convert from other type. */
    time_t          unix_creation_stamp;
    time_t          unix_last_modified_stamp;
    time_t          unix_last_access_stamp;
    unsigned short  unix_mode;
    unsigned short  unix_uid;
    unsigned short  unix_gid;
	
	std::string     user;
	std::string     group;
    //char            user[256];
    //char            group[256];
}  LzHeader;


// indices of values of header in file
enum tHeaderIndices
{
	I_HEADER_SIZE          = 0,     /* level 0,1,2   */
	I_HEADER_CHECKSUM      = 1,     /* level 0,1     */
	I_METHOD               = 2,     /* level 0,1,2,3 */
	I_PACKED_SIZE          = 7,     /* level 0,1,2,3 */
	I_ATTRIBUTE            = 19,    /* level 0,1,2,3 */
	I_HEADER_LEVEL         = 20,    /* level 0,1,2,3 */
	COMMON_HEADER_SIZE     = 21,    /* size of common part */
	I_GENERIC_HEADER_SIZE  = 24,    /* + name_length */
	I_LEVEL0_HEADER_SIZE   = 36,    /* + name_length (unix extended) */
	I_LEVEL1_HEADER_SIZE   = 27,    /* + name_length */
	I_LEVEL2_HEADER_SIZE   = 26,    /* + padding */
	I_LEVEL3_HEADER_SIZE   = 32
};


class CLzHeader
{
private:
	// TODO: these buffer handlings REALLY need to be fixed..
	// change methods later, move to buffer-class
	//
	char    *m_get_ptr;
	char    *m_get_ptr_end;
	
	char    *m_put_ptr;
	
	inline int get_byte()
	{
		return *m_get_ptr++ & 0xff;
	}
	inline void skip_bytes(size_t len)
	{
		m_get_ptr += (len);
	}
	inline void put_byte(int c)
	{
		*m_put_ptr++ = (char)(c);
	}

	inline int get_word()
	{
		int b0 = get_byte();
		int b1 = get_byte();
		int w = (b1 << 8) + b0;
		return w;
	}
	
	inline void put_word(unsigned int v)
	{
		put_byte(v);
		put_byte(v >> 8);
	}

	inline long get_longword()
	{
		long b0 = get_byte();
		long b1 = get_byte();
		long b2 = get_byte();
		long b3 = get_byte();
		return (b3 << 24) + (b2 << 16) + (b1 << 8) + b0;
	}
	
	inline void put_longword(long v)
	{
		put_byte(v);
		put_byte(v >> 8);
		put_byte(v >> 16);
		put_byte(v >> 24);
	}

	inline int get_bytes(char *buf, int len, int size)
	{
		int i = 0;
		for (; i < len && i < size; i++)
		{
			buf[i] = m_get_ptr[i];
		}
		m_get_ptr += len;
		return i;
	}
	
	inline void put_bytes(char *buf, int len)
	{
		for (int i = 0; i < len; i++)
		{
			put_byte(buf[i]);
		}
	}
	
	std::string get_string(int len)
	{
		std::string szVal;
		szVal.assign(m_get_ptr, len);
		//szVal.assign(len, 0x00);
		return szVal;
	}

	time_t generic_to_unix_stamp(long t)
	{
		CGenericTime Time(t);
		return (time_t)Time;
	}
	
	long unix_to_generic_stamp(time_t t)
	{
		CGenericTime Time(t);
		return (long)Time;
	}
	
	unsigned long wintime_to_unix_stamp()
	{
		unsigned long ulHiPart = (unsigned long)get_longword();
		unsigned long ulLoPart = (unsigned long)get_longword();
		
		CFiletimeHelper ft(ulHiPart, ulLoPart);
		return ft.GetAsUnixTime();
	}

	// TODO: make better way..
	//size_t m_nReadOffset;
	
	
	
protected:
	tHeaderLevel m_enHeaderLevel;
	int m_iHeaderSize;
	CCrcIo m_crcio;
	
	inline int calc_sum(unsigned char *p, size_t len) const
	{
		int sum = 0;
		while (len--) 
		{
			sum += *p++;
		}
		return sum & 0xff;
	}
	

public:
	CLzHeader(void)
		: m_iHeaderSize(0)
		//, m_nReadOffset(0)
		, m_get_ptr(nullptr)
		, m_get_ptr_end(nullptr)
		, m_crcio()
	{}
	~CLzHeader(void)
	{}

	// If true, archive is msdos SFX (executable)
	bool IsMsdosSFX1(CReadBuffer &Buffer) const
	{
		// "MZ..", 0x4D5A9000
		// -> MSDOS, OS/2 or MS-Windows executable/DLL
		//
		unsigned char *p = Buffer.GetBegin();
		if (p[0] == 0x4D && p[1] == 0x5A && p[2] == 0x90 && p[3] == 0x00)
		{
			// identifier for executable/DLL found
			return true;
		}
		return false;
	}

	// If true, archive is Amiga 'run' (executable)
	bool IsAmigaRun(CReadBuffer &Buffer) const
	{
		// 0x000003F3 -> AmigaDOS executable
		unsigned char *p = Buffer.GetBegin();
		if (p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x03 && p[3] == 0xF3)
		{
			return true;
		}
		// 0x000003E7 -> AmigaDOS library
		if (p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x03 && p[3] == 0xE7)
		{
			return true;
		}
		return false;
	}
	
	// if file is self-decompressing exectuable,
	// we need to seek to actual archive header..
	unsigned char *SeekArchiveHeader(CReadBuffer &Buffer) const
	{
		unsigned char *p = Buffer.GetBegin();
		unsigned char *pEnd = Buffer.GetEnd();
		
		bool bHeaderFound = false;
		size_t nPos = 0;
		while (p < pEnd && bHeaderFound == false)
		{
			// -l??- -> should be archive-header
			if (! (p[I_METHOD] == '-' && p[I_METHOD+1]=='l' && p[I_METHOD+4]=='-'))
			{
				bHeaderFound = true;
				break;
			}
			p = p+nPos;
		}
		
		if (bHeaderFound == true)
		{
			// 
			return p;
		}
		
		// not found -> not archive?
		return nullptr;
	}
	
	// is valid lha type: verify "-l??-" keyword
	// (e.g. -lh5- -lz0- etc.)
	//
	bool IsValidLha(CReadBuffer &Buffer) const
	{
		bool bHeaderValid = false;

		// usually we can start from beginning
		unsigned char *p = Buffer.GetBegin();
		
		// executable (self-decompressing)
		// -> seek actual archive-header
		if (IsMsdosSFX1(Buffer) == true)
		{
			p = SeekArchiveHeader(Buffer);
			if (p == nullptr)
			{
				// not found -> not archive?
				return false;
			}
			// -> now do actual checking
		}

		// -lh?- or -lz?- where '?' = 0..7
		if (! (p[I_METHOD] == '-' && p[I_METHOD+1] == 'l' && p[I_METHOD+4] == '-'))
			   //&& (p[I_METHOD+2] == 'h' || p[I_METHOD+2] == 'z')))
		{
			// non-valid identifier
			return false;
		}

        // level 0 or 1 header 
        if ((p[I_HEADER_LEVEL] == 0 || p[I_HEADER_LEVEL] == 1)
            && p[I_HEADER_SIZE] > 20
            && p[I_HEADER_CHECKSUM] == calc_sum(p+2, p[I_HEADER_SIZE])) 
		{
			bHeaderValid = true;
        }

        // level 2 header 
        if (p[I_HEADER_LEVEL] == 2
            && p[I_HEADER_SIZE] >= 24
            && p[I_ATTRIBUTE] == 0x20) 
		{
			bHeaderValid = true;
        }
		
		return bHeaderValid;
	}

	// parse archive-header from buffer
	// (keep necessary values for later)
	//
	bool ParseBuffer(CReadBuffer &Buffer)
	{
		unsigned char *p = Buffer.GetBegin();
		unsigned char *pEnd = Buffer.GetEnd();
		
		m_enHeaderLevel = (tHeaderLevel)(p[I_HEADER_LEVEL]);
		m_iHeaderSize = p[I_HEADER_SIZE];
		
		return true;
	}
	
	LzHeader *GetNextHeader(CReadBuffer &Buffer, CAnsiFile &ArchiveFile);
	
protected:
	//ssize_t get_extended_header(CAnsiFile &ArchiveFile, LzHeader *pHeader, CReadBuffer &Buffer, size_t header_size, unsigned int *hcrc);
	size_t get_extended_header(CAnsiFile &ArchiveFile, LzHeader *pHeader, CReadBuffer &Buffer, size_t header_size, unsigned int *hcrc);
	
	bool get_header_level0(CAnsiFile &ArchiveFile, LzHeader *pHeader, CReadBuffer &Buffer);
	bool get_header_level1(CAnsiFile &ArchiveFile, LzHeader *pHeader, CReadBuffer &Buffer);
	bool get_header_level2(CAnsiFile &ArchiveFile, LzHeader *pHeader, CReadBuffer &Buffer);
	bool get_header_level3(CAnsiFile &ArchiveFile, LzHeader *pHeader, CReadBuffer &Buffer);
	
	//void init_header(LzHeader *pHeader);
};


#endif // LZHEADER_H
