//////////////////////////////////
//
// structure and code for handling archive-file header:
// there are various different headers and extensions
//
// Ilkka Prusi 2011
//

#ifndef LZHEADER_H
#define LZHEADER_H

#include <QObject>
#include <QString>
#include <QTextCodec>
#include <QList>
#include <QDateTime>

#include "AnsiFile.h"
#include "LhaTypeDefs.h"

#include "crcio.h"
#include "GenericTime.h"
#include "FiletimeHelper.h"

#define METHOD_TYPE_STORAGE     5

// write open the bit-flags for in/out of library

typedef struct MsdosFlags
{
	// constructor
	MsdosFlags()
	{
		SetFromValue(0x20);
	}
	
	// some shitty MS-DOS-style flags:
	bool bRo;  // bit1  read only
	bool bHid; // bit2  hidden
	bool bSys; // bit3  system
	bool bVol; // bit4  volume label
	bool bDir; // bit5  directory
	bool bArc; // bit6  archive bit (need to backup)
	
	void SetFromValue(unsigned char ucVal)
	{
		bRo = ((ucVal & 2) ? true : false);
		bHid = ((ucVal & 4) ? true : false);
		bSys = ((ucVal & 8) ? true : false);
		bVol = ((ucVal & 16) ? true : false);
		bDir = ((ucVal & 32) ? true : false);
		bArc = ((ucVal & 64) ? true : false);
	}
	
} MsdosFlags;


enum tUnixFlags
{
	UNIX_FILE_TYPEMASK     = 0170000,
	UNIX_FILE_REGULAR      = 0100000,
	UNIX_FILE_DIRECTORY    = 0040000,
	UNIX_FILE_SYMLINK      = 0120000,
	UNIX_SETUID            = 0004000,
	UNIX_SETGID            = 0002000,
	UNIX_STICKYBIT         = 0001000,
	UNIX_OWNER_READ_PERM   = 0000400,
	UNIX_OWNER_WRITE_PERM  = 0000200,
	UNIX_OWNER_EXEC_PERM   = 0000100,
	UNIX_GROUP_READ_PERM   = 0000040,
	UNIX_GROUP_WRITE_PERM  = 0000020,
	UNIX_GROUP_EXEC_PERM   = 0000010,
	UNIX_OTHER_READ_PERM   = 0000004,
	UNIX_OTHER_WRITE_PERM  = 0000002,
	UNIX_OTHER_EXEC_PERM   = 0000001,
	UNIX_RW_RW_RW          = 0000666
};

typedef struct UnixModeFlags
{
	//tUnixFlags m_enFlags;
	unsigned short  unix_mode;
	
	// constructor
	UnixModeFlags()
	{
		// default: normal file with read+write for everyone
		unix_mode = UNIX_FILE_REGULAR | UNIX_RW_RW_RW;
	}

	// is file just symbolic link instead of actual file?
	bool IsSymLink()
	{
		// TODO: UNIX_FILE_TYPEMASK ?
		if ((unix_mode & UNIX_FILE_SYMLINK) == UNIX_FILE_SYMLINK)
		{
			return true;
		}
		return false;
	}
	
	bool IsDirectory()
	{
		// TODO: UNIX_FILE_TYPEMASK ?
		if ((unix_mode & UNIX_FILE_DIRECTORY) == UNIX_FILE_DIRECTORY)
		{
			return true;
		}
		return false;
	}

} UnixModeFlags;

typedef struct LzHeader 
{
	// constructor
	LzHeader()
	{
		/* the `method' member is rewrote by the encoding function.
		   but need set for empty files */
		memcpy(method, LZHUFF0_METHOD, METHOD_TYPE_STORAGE);
		
		packed_size = 0;
		original_size = 0;
		header_level = 0;
		
		crc = 0x0000;
		extend_type = EXTEND_UNIX;
		
		data_pos = 0;
		
		// defaults for these
		//unix_mode = UNIX_FILE_REGULAR | UNIX_RW_RW_RW;
		unix_gid = 0;
		unix_uid = 0;
	}
	
    size_t          header_size;
    int             size_field_length; // "size" variable may have different sizes??
    char            method[METHOD_TYPE_STORAGE];
    size_t          packed_size;
    size_t          original_size;
    unsigned char   header_level;
	QString         filename;
	QString         dirname;
	QString         realname; // real name for symbolic link (unix)

	// note: only 16-bit crc..
    unsigned int    crc;      /* file CRC */
    bool            has_crc;  /* file CRC */
    unsigned int    header_crc; /* header CRC */
	
    unsigned char   extend_type;
	
	// keep offset of data in file for locating later..
	long            data_pos;

	// MS-DOS attribute-flags
    MsdosFlags      MsDosAttributes;
	
    /* extend_type == EXTEND_UNIX  and convert from other type. */
	
    QDateTime       creation_stamp;
    QDateTime       last_modified_stamp;
    QDateTime       last_access_stamp;

	// only on unix-extension ?
    unsigned char   minor_version;
	
	UnixModeFlags   UnixMode;
    unsigned short  unix_uid;
    unsigned short  unix_gid;
	
	QString         user;
	QString         group;

	bool IsMethod(char Method[METHOD_TYPE_STORAGE])
	{
		if (memcmp(method, Method, METHOD_TYPE_STORAGE) == 0)
		{
			return true;
		}
		return false;
	}
	
	// get suitable method for extraction:
	// check string if it is supported.
	//
	// note: there are some variations..
	//
	tCompressionMethod GetMethod()
	{
		if (IsMethod(LZHUFF0_METHOD) == true)
		{
			return LZHUFF0_METHOD_NUM;
		}
		else if (IsMethod(LZHUFF1_METHOD) == true)
		{
			return LZHUFF1_METHOD_NUM;
		}
		else if (IsMethod(LZHUFF2_METHOD) == true)
		{
			return LZHUFF2_METHOD_NUM;
		}
		else if (IsMethod(LZHUFF3_METHOD) == true)
		{
			return LZHUFF3_METHOD_NUM;
		}
		else if (IsMethod(LZHUFF4_METHOD) == true)
		{
			return LZHUFF4_METHOD_NUM;
		}
		else if (IsMethod(LZHUFF5_METHOD) == true)
		{
			return LZHUFF5_METHOD_NUM;
		}
		else if (IsMethod(LZHUFF6_METHOD) == true)
		{
			return LZHUFF6_METHOD_NUM;
		}
		else if (IsMethod(LZHUFF7_METHOD) == true)
		{
			return LZHUFF7_METHOD_NUM;
		}
		else if (IsMethod(LARC_METHOD) == true)
		{
			return LARC_METHOD_NUM;
		}
		else if (IsMethod(LARC5_METHOD) == true)
		{
			return LARC5_METHOD_NUM;
		}
		else if (IsMethod(LARC4_METHOD) == true)
		{
			return LARC4_METHOD_NUM;
		}
		else if (IsMethod(LZHDIRS_METHOD) == true)
		{
			return LZHDIRS_METHOD_NUM;
		}
		
		// unknown/unsupported
		return LZ_UNKNOWN;
	}
	
	
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

// extended header attributes:
// used to parse attributes from archive-file
enum tExtendedAttribs
{
	EXTH_CRC      = 0,
	EXTH_FILENAME = 1,
	EXTH_PATH     = 2,
	
	EXTH_COMMENT          = 0x3f, // file comment (Amiga-style?), uncompressed
	EXTH_MSDOSATTRIBS     = 0x40,
	EXTH_WINDOWSTIMES     = 0x41, // Windows timestamps of file
	EXTH_LARGEFILE        = 0x42, // 64-bit filesize
	EXTH_UNIXPERMISSIONS  = 0x50, // UNIX permission
	EXTH_UNIXGIDUID       = 0x51, // UNIX gid and uid
	EXTH_UNIXGROUP        = 0x52, // UNIX group name
	EXTH_UNIXUSER         = 0x53, // UNIX user name
	EXTH_UNIXLASTMODIFIED = 0x54, // UNIX last modified time 
	
	EXTH_RESERVED         = 0xFF // reservation for later
};

class CLhHeader : public QObject
{
private:
	
	// TODO: these buffer handlings REALLY need to be fixed..
	// change methods later, move to buffer-class
	//
	// temp! buffer-descriptor
	//
	/*
	typedef struct LhBuffer
	{
		
	} LhBuffer;
	*/
	
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
	
	/*
	inline void put_byte(int c)
	{
		*m_put_ptr++ = (char)(c);
	}
	*/

	inline int get_word()
	{
		int b0 = get_byte();
		int b1 = get_byte();
		int w = (b1 << 8) + b0;
		return w;
	}
	
	/*
	inline void put_word(unsigned int v)
	{
		put_byte(v);
		put_byte(v >> 8);
	}
	*/

	inline long get_longword()
	{
		long b0 = get_byte();
		long b1 = get_byte();
		long b2 = get_byte();
		long b3 = get_byte();
		return (b3 << 24) + (b2 << 16) + (b1 << 8) + b0;
	}

	/*
	inline void put_longword(long v)
	{
		put_byte(v);
		put_byte(v >> 8);
		put_byte(v >> 16);
		put_byte(v >> 24);
	}
	*/

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
	
	/*
	inline void put_bytes(char *buf, int len)
	{
		for (int i = 0; i < len; i++)
		{
			put_byte(buf[i]);
		}
	}
	*/
	
	// note: string isn't null-terminated in file
	// so we need to work around that..
	//
	// note: read-buffer is deallocated/overwritten
	// later when next chunk is read/done.
	//
	QString get_string(int len)
	{
		QString szVal;
		if (m_pTextCodec == nullptr)
		{
			szVal = QString::fromAscii(m_get_ptr, len);
		}
		else
		{
			szVal = m_pTextCodec->toUnicode(m_get_ptr, len);
		}
		m_get_ptr += len;
		return szVal;
	}

	CFiletimeHelper get_wintime()
	{
		unsigned long ulHiPart = (unsigned long)get_longword();
		unsigned long ulLoPart = (unsigned long)get_longword();
		
		CFiletimeHelper ft(ulHiPart, ulLoPart);
		return ft;
	}
	

	// TODO: make better way..
	//size_t m_nReadOffset;
	
	
	
protected:
	tHeaderLevel m_enHeaderLevel;
	int m_iHeaderSize;
	
	typedef QList<LzHeader*> tFileList;
	tFileList m_HeaderList;
	
	CCrcIo m_crcio;
	
	QTextCodec *m_pTextCodec;
	CReadBuffer *m_pReadBuffer;

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
	CLhHeader(QObject *parent = 0)
		: QObject(parent)
		, m_iHeaderSize(0)
		//, m_nReadOffset(0)
		, m_get_ptr(nullptr)
		, m_get_ptr_end(nullptr)
		, m_HeaderList()
		, m_crcio()
		, m_pTextCodec(nullptr)
		, m_pReadBuffer(nullptr)
	{}
	~CLhHeader(void)
	{
		// cleanup when destroyed
		Clear();
	}
	
	void Clear()
	{
		auto it = m_HeaderList.begin();
		auto itEnd = m_HeaderList.end();
		while (it != itEnd)
		{
			LzHeader *pHeader = (*it);
			delete pHeader;
			
			++it;
		}
		m_HeaderList.clear();
	}
	
	void SetConversionCodec(QTextCodec *pCodec)
	{
		m_pTextCodec = pCodec;
	}

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
		//unsigned char *pEnd = Buffer.GetEnd();
		
		m_enHeaderLevel = (tHeaderLevel)(p[I_HEADER_LEVEL]);
		m_iHeaderSize = p[I_HEADER_SIZE];
		
		return true;
	}
	
	void ParseHeaders(CAnsiFile &ArchiveFile);

	
protected:
	size_t get_extended_header(CAnsiFile &ArchiveFile, LzHeader *pHeader, size_t header_size, unsigned int *hcrc);
	
	bool get_header_level0(CAnsiFile &ArchiveFile, LzHeader *pHeader);
	bool get_header_level1(CAnsiFile &ArchiveFile, LzHeader *pHeader);
	bool get_header_level2(CAnsiFile &ArchiveFile, LzHeader *pHeader);
	bool get_header_level3(CAnsiFile &ArchiveFile, LzHeader *pHeader);
	
	void UpdatePaddingToCrc(CAnsiFile &ArchiveFile, unsigned int &hcrc, const long lPadSize);
	
	friend class CLhArchive;
};


#endif // LZHEADER_H
