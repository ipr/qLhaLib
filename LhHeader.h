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
#include <QByteArray>
#include <QTextCodec>
#include <QList>
#include <QDateTime>

#include <stdint.h>

#include "AnsiFile.h"
#include "LhaTypeDefs.h"

#include "crcio.h"
#include "GenericTime.h"
#include "FiletimeHelper.h"

// misc. helper-structures for file-modes
#include "FilemodeFlags.h"

// renamed for clarity..
#define PACKMETHOD_TYPE_LENGTH ((int)5)


class LzHeader 
{
public:
	// constructor
	LzHeader()
		: m_enCompression(LZ_UNKNOWN)
		, pack_method()
		, header_size(0)
		, extend_size(0)
		, size_field_length(0)
		, packed_size(0)
		, original_size(0)
		, header_level(0)
		, header_pos(0)
		, data_pos(0)
		, extend_type(EXTEND_UNIX)
		, filename()
		, dirname()
		, realname()
		, file_comment()
		, UnixMode()
	{
		crc = 0x0000;
		has_crc = false;
		header_crc = 0;
		
		// defaults for these
		unix_gid = 0;
		unix_uid = 0;
	}
	
    size_t          header_size;
	long            extend_size; // size of extended-header (if any)
    int             size_field_length; // "size" variable may have different sizes??
	
	QString         pack_method; // method as-is from file, e.g. -lh0-..-lh7-, -lhd-, -lzs-, -lz5-, -lz4-
    tCompressionMethod m_enCompression; // enumeration of supported types
	
    size_t          packed_size;
    size_t          original_size;
    unsigned char   header_level; // level/type of header (0..3)
    
	QString         filename;
	QString         dirname;
	QString         realname; // real name for symbolic link (unix)
	
	QString			file_comment; // usually Amiga-packed files have comment also

	// note: only 16-bit crc..
    unsigned int    crc;      /* file CRC */
    bool            has_crc;  /* file CRC */
    unsigned int    header_crc; /* header CRC */
	
    unsigned char   extend_type; /* OS type, single character */
	
	// keep offset of data in file for locating later..
	long            header_pos;
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


	// get suitable method for extraction:
	// check string if it is supported.
	//
	tCompressionMethod GetMethod()
	{
		if (pack_method.contains("-lh") == true
			&& pack_method.at(4) == '-')
		{
			// -lh?-
			// -> LZHUFF, get level..
			const char level = pack_method.at(3).toAscii();
			switch (level)
			{
			case '0':
				return LZHUFF0_METHOD_NUM;
			case '1':
				return LZHUFF1_METHOD_NUM;
			case '2':
				return LZHUFF2_METHOD_NUM;
			case '3':
				return LZHUFF3_METHOD_NUM;
			case '4':
				return LZHUFF4_METHOD_NUM;
			case '5':
				return LZHUFF5_METHOD_NUM;
			case '6':
				return LZHUFF6_METHOD_NUM;
			case '7':
				return LZHUFF7_METHOD_NUM;
				
				/* 
			case '8':
				// -lh8- same as -lh7- ?
				// was it ever in use?
				return LZHUFF7_METHOD_NUM;
				*/
				
			case 'd': // -lhd-
				return LZHDIRS_METHOD_NUM;
				
				/*
			case '9': // -lh9-
			case 'a': // -lha-
			case 'b': // -lhb-
			case 'c': // -lhc-
			case 'e': // -lhe-
				// Joe Jared extensions, not yet supported..
				break;
				*/
				
				/*
			case 'x': // -lhx-
				// UNLHA32 ?
				break;
				*/
			}
		}
		else if (pack_method.contains("-lz") == true
			&& pack_method.at(4) == '-')
		{
			// -lz?-
			// LhArc, get level..
			const char level = pack_method.at(3).toAscii();
			switch (level)
			{
			case 's': // -lzs-
				return LARC_METHOD_NUM;
			case '5': // -lz5-
				return LARC5_METHOD_NUM;
			case '4': // -lz4-
				return LARC4_METHOD_NUM;

				/*
			case '7': // -lz7-
			case '8': // -lz8-
			case '2': // -lz2-
			case '3': // -lz3-
				// LArc extensions, not yet supported..
				break;
				*/
			}
		}
		/* 
		// CP/M variations?
		else if (pack_method.contains("-pm") == true
			&& pack_method.at(4) == '-')
		{
		}
		else if (pack_method == "-pc1-")
		{
		}
		*/

		// fallthrough: unknown/unsupported
		return LZ_UNKNOWN;
	}
	
	QString GetOSTypeName()
	{
		switch (extend_type)
		{
		case 0:
			return "Generic";
		case 'U':
			return "Unix";
		case 'M':
			return "MSDOS";
		case 'm':
			return "MacOS";
		case '2':
			return "OS/2";
		case '9':
			return "OS9";
		case 'K':
			return "OS/68K";
		case '3': /* OS-9000 ?? */
			return "OS/386";
		case 'H':
			return "HUMAN";
		case 'C':
			return "CP/M";
		case 'F':
			return "FLEX";
		case 'w':
			return "Winslows 95/98";
		case 'W':
			return "Winslows NT";
		case 'R':
			return "Runser";
		case 'T':
			// not official..
			return "Townsos";
		case 'X': /* OS-9 for X68000 ?*/
			// not official..
			return "XOSK";
		case 'J':
			// not official..
			return "Java";
		}
		return "Unknown";
	}
};


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
	Q_OBJECT

private:
	
	
	// TODO: these buffer handlings REALLY need to be fixed..
	// change methods later, move to buffer-class
	//
	char    *m_get_ptr;
	
	inline int get_byte()
	{
		return *m_get_ptr++ & 0xff;
	}
	
	inline void skip_bytes(size_t len)
	{
		m_get_ptr += (len);
	}
	
	inline int get_word()
	{
		int b0 = get_byte();
		int b1 = get_byte();
		int w = (b1 << 8) + b0;
		return w;
	}
	
	inline long get_longword()
	{
		long b0 = get_byte();
		long b1 = get_byte();
		long b2 = get_byte();
		long b3 = get_byte();
		return (b3 << 24) + (b2 << 16) + (b1 << 8) + b0;
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
	
	
	// note: string isn't null-terminated in file.
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
	
	// get pathname (file/dir) and fix path-separator:
	// don't modify source-buffer as 
	// that affects CRC-calculation..
	// 	
	QString getPathname(int len, bool fixPath = true)
	{
		QByteArray tmp(m_get_ptr, len);
		
		for (int i = 0; i < len; i++)
		{
			// fix '\\' path-separator 
			// since it may be handled as Yen-sign
			// after conversion to Unicode..
			if (tmp[i] == (char)0xFF
				&& fixPath == true)
			{
				tmp[i] = '/';
			}
		}
		m_get_ptr += len;
		
		if (m_pTextCodec == nullptr)
		{
			return QString(tmp);
		}
		else
		{
			return m_pTextCodec->toUnicode(tmp);
		}
	}
	
	// get string upto NULL (if found),
	// used in case there are filename and file-comment
	// in same string (use them separately)
	//
	int getStringToNULL(int len, QString &string, bool fixPath = false)
	{
		for (int i = 0; i < len; i++)
		{
			// fix '\\' path-separator before reading
			// since Unicode-conversion may assume it is Yen-sign..
			//
			if (m_get_ptr[i] == (char)0xFF
				&& fixPath == true)
			{
				m_get_ptr[i] = '/';
			}
		
			// if there is NULL on the string
			// we should stop reading there:
			// in case of filename there may be file-comment after that
			// (such as Amiga file comment)
			// 
			if (m_get_ptr[i] == 0x00)
			{
				len = i;
				break;
			}
		}
		
		if (m_pTextCodec == nullptr)
		{
			string = QString::fromAscii(m_get_ptr, len);
		}
		else
		{
			string = m_pTextCodec->toUnicode(m_get_ptr, len);
		}
		m_get_ptr += len;
		return len;
	}

	CFiletimeHelper get_wintime()
	{
		unsigned long ulHiPart = (unsigned long)get_longword();
		unsigned long ulLoPart = (unsigned long)get_longword();
		
		CFiletimeHelper ft(ulHiPart, ulLoPart);
		return ft;
	}
	

protected:
	
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
		, m_get_ptr(nullptr)
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

	void ParseHeaders(CAnsiFile &ArchiveFile);

signals:
	// progress-status by signals, errors by exceptions
	void message(QString);
	void warning(QString);
	
protected:

	bool get_header_level0(CAnsiFile &ArchiveFile, LzHeader *pHeader);
	bool get_header_level1(CAnsiFile &ArchiveFile, LzHeader *pHeader);
	bool get_header_level2(CAnsiFile &ArchiveFile, LzHeader *pHeader);
	bool get_header_level3(CAnsiFile &ArchiveFile, LzHeader *pHeader);

	size_t get_extended_header(CAnsiFile &ArchiveFile, LzHeader *pHeader, unsigned int *hcrc);
	
	int readFilenameComment(CAnsiFile &ArchiveFile, LzHeader *pHeader, const int name_length);
	
	void UpdatePaddingToCrc(CAnsiFile &ArchiveFile, unsigned int &hcrc, const long lPadSize);
	
	friend class CLhArchive;
};


#endif // LZHEADER_H
