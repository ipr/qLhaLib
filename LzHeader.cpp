//////////////////////////////////
//
// structure and code for handling Lz-archive header
//

#include "LzHeader.h"

#include <QString>


LzHeader *CLzHeader::GetNextHeader(CReadBuffer &Buffer, CAnsiFile &ArchiveFile)
{
    //int archive_kanji_code = CODEPAGE_SJIS;
    //int system_kanji_code = default_system_kanji_code;
    char *archive_delim = "\377\\"; /* `\' is for level 0 header and broken archive. */
    char *system_delim = "//";
    int filename_case = CODEPAGE_NONE;

	m_get_ptr = (char*)Buffer.GetBegin();
	m_get_ptr_end = (char*)Buffer.GetEnd();
	
    int end_mark = getc((FILE*)ArchiveFile);
    if (end_mark == EOF || end_mark == 0) 
	{
        return nullptr;           /* finish */
    }
    m_get_ptr[0] = end_mark;

    if (ArchiveFile.Read(m_get_ptr + 1, COMMON_HEADER_SIZE - 1) == false) 
	{
        throw ArcException("Invalid header (LHarc file ?)", "");
    }

	LzHeader *pHeader = new LzHeader();
	bool bRet = false;
    switch (m_get_ptr[I_HEADER_LEVEL]) 
	{
    case 0:
		bRet = get_header_level0(ArchiveFile, pHeader, Buffer);
        break;
    case 1:
		bRet = get_header_level1(ArchiveFile, pHeader, Buffer);
        break;
    case 2:
		bRet = get_header_level2(ArchiveFile, pHeader, Buffer);
        break;
    case 3:
		bRet = get_header_level3(ArchiveFile, pHeader, Buffer);
        break;
    default:
		throw ArcException("Unknown level header", m_get_ptr[I_HEADER_LEVEL]);
    }
	
	if (bRet == false)
	{
        throw ArcException("Failure reading header", "");
	}

    /* filename conversion */
    switch (pHeader->extend_type) 
	{
    case EXTEND_MSDOS:
        filename_case = CODEPAGE_NONE;
        break;
    case EXTEND_HUMAN:
    case EXTEND_OS68K:
    case EXTEND_XOSK:
    case EXTEND_UNIX:
    case EXTEND_JAVA:
        filename_case = CODEPAGE_NONE;
        break;

    case EXTEND_MACOS:
        archive_delim = "\377/:\\"; // `\' is for level 0 header and broken archive. 
        system_delim = "/://";
        filename_case = CODEPAGE_NONE;
        break;

    default:
        filename_case = CODEPAGE_NONE;
        break;
    }

	/*
    if (optional_archive_kanji_code)
        archive_kanji_code = optional_archive_kanji_code;
    if (optional_system_kanji_code)
        system_kanji_code = optional_system_kanji_code;
    if (optional_archive_delim)
        archive_delim = optional_archive_delim;
    if (optional_system_delim)
        system_delim = optional_system_delim;
    if (optional_filename_case)
        filename_case = optional_filename_case;
		*/

    /* kanji code and delimiter conversion */
	/*
    convert_filename(pHeader->name, strlen(pHeader->name), sizeof(pHeader->name),
                     archive_kanji_code,
                     system_kanji_code,
                     archive_delim, 
					 system_delim, 
					 filename_case);*/

    if ((pHeader->unix_mode & UNIX_FILE_SYMLINK) == UNIX_FILE_SYMLINK) 
	{
		/* hdr->name is symbolic link name */
		/* hdr->realname is real name */
		std::string::size_type nPos = pHeader->name.find('|');
		if (nPos != std::string::npos)
		{
			pHeader->realname = pHeader->name.substr(nPos+1);
			
			//nPos = pHeader->name.find('|', nPos);
		}
        else
		{
			throw ArcException("Unknown symlink name", pHeader->name);
		}
    }

    return pHeader;
}


/* ------------------------------------------------------------------------ */
/* LHa for UNIX                                                             */
/*              header.c -- header manipulate functions                     */
/*                                                                          */
/*      Modified                Nobutaka Watazaki                           */
/*                                                                          */
/*  Original                                                Y.Tagawa        */
/*  modified                                    1991.12.16  M.Oki           */
/*  Ver. 1.10  Symbolic Link added              1993.10.01  N.Watazaki      */
/*  Ver. 1.13b Symbolic Link Bug Fix            1994.08.22  N.Watazaki      */
/*  Ver. 1.14  Source All chagned               1995.01.14  N.Watazaki      */
/*  Ver. 1.14i bug fixed                        2000.10.06  t.okamoto       */
/*  Ver. 1.14i Contributed UTF-8 convertion for Mac OS X                    */
/*                                              2002.06.29  Hiroto Sakai    */
/*  Ver. 1.14i autoconfiscated & rewritten      2003.02.23  Koji Arai       */
/* ------------------------------------------------------------------------ */
//#include "lha_main.h"
/*

// CODE_EUC, CODE_SJIS, CODE_UTF8, CODE_CAP
tKanjiCodePage optional_archive_kanji_code = CODEPAGE_NONE;

// CODE_EUC, CODE_SJIS, CODE_UTF8, CODE_CAP
tKanjiCodePage optional_system_kanji_code = CODEPAGE_NONE;

char *optional_archive_delim = NULL;
char *optional_system_delim = NULL;
int optional_filename_case = CODEPAGE_NONE;

#ifdef MULTIBYTE_FILENAME
tKanjiCodePage default_system_kanji_code = MULTIBYTE_FILENAME;
#else
tKanjiCodePage default_system_kanji_code = CODEPAGE_NONE;
#endif

*/


/*
 * extended header
 *
 *             size  field name
 *  --------------------------------
 *  base header:         :
 *           2 or 4  next-header size  [*1]
 *  --------------------------------------
 *  ext header:   1  ext-type            ^
 *                ?  contents            | [*1] next-header size
 *           2 or 4  next-header size    v
 *  --------------------------------------
 *
 *  on level 1, 2 header:
 *    size field is 2 bytes
 *  on level 3 header:
 *    size field is 4 bytes
 */

//static ssize_t
size_t CLzHeader::get_extended_header(CAnsiFile &ArchiveFile, LzHeader *pHeader, CReadBuffer &Buffer, size_t header_size, unsigned int *hcrc)
{
    size_t whole_size = header_size;
    int n = 1 + pHeader->size_field_length; /* `ext-type' + `next-header size' */

    if (pHeader->header_level == 0)
	{
        return 0;
	}

    while (header_size) 
	{
		m_get_ptr = (char*)Buffer.GetBegin();
		
        if (Buffer.GetSize() < header_size) 
		{
			throw ArcException("header size too large.", header_size);
        }

        if (ArchiveFile.Read(Buffer.GetBegin(), header_size) == false) 
		{
			throw ArcException("Invalid header (LHa file ?)", 0);
        }

		unsigned char *pBuf = Buffer.GetBegin();
		
        int ext_type = get_byte();
        switch (ext_type) 
		{
        case 0:
            /* header crc (CRC-16) */
            pHeader->header_crc = get_word();
            /* clear buffer for CRC calculation. */
            pBuf[1] = pBuf[2] = 0;
            skip_bytes(header_size - n - 2);
            break;
        case 1:
            /* filename */
            //name_length = get_bytes(pHeader->name, header_size-n, sizeof(pHeader->name)-1);
            pHeader->name = get_string(header_size-n);
            break;
        case 2:
            /* directory */
            //dir_length = get_bytes(dirname, header_size-n, sizeof(dirname)-1);
            pHeader->dirname = get_string(header_size-n);
            break;
        case 0x40:
            /* MS-DOS attribute */
            pHeader->attribute = get_word();
            break;
        case 0x41:
			{
				/* Windows time stamp (FILETIME structure) */
				/* it is time in 100 nano seconds since 1601-01-01 00:00:00 */
				
				pHeader->unix_creation_stamp = wintime_to_unix_stamp();
				time_t LastModifiedTime = wintime_to_unix_stamp();
				pHeader->unix_last_access_stamp = wintime_to_unix_stamp();
				
				/* set last modified time */
				//if (pHeader->header_level >= 2)  /* time_t has been already set */
				if (pHeader->header_level < 2)
				{
					pHeader->unix_last_modified_stamp = LastModifiedTime;
				}
			}
			break;
        case 0x50:
            /* UNIX permission */
            pHeader->unix_mode = get_word();
            break;
        case 0x51:
            /* UNIX gid and uid */
            pHeader->unix_gid = get_word();
            pHeader->unix_uid = get_word();
            break;
        case 0x52:
            /* UNIX group name */
            //i = get_bytes(pHeader->group, header_size-n, sizeof(pHeader->group)-1);
            pHeader->group = get_string(header_size-n);
            break;
        case 0x53:
            /* UNIX user name */
            //i = get_bytes(pHeader->user, header_size-n, sizeof(pHeader->user)-1);
            pHeader->user = get_string(header_size-n);
            break;
        case 0x54:
            /* UNIX last modified time */
            pHeader->unix_last_modified_stamp = (time_t) get_longword();
            break;
        default:
            /* other headers */
            /* 0x39: multi-disk header
               0x3f: uncompressed comment
               0x42: 64bit large file size
               0x48-0x4f(?): reserved for authenticity verification
               0x7d: encapsulation
               0x7e: extended attribute - platform information
               0x7f: extended attribute - permission, owner-id and timestamp
                     (level 3 on OS/2)
               0xc4: compressed comment (dict size: 4096)
               0xc5: compressed comment (dict size: 8192)
               0xc6: compressed comment (dict size: 16384)
               0xc7: compressed comment (dict size: 32768)
               0xc8: compressed comment (dict size: 65536)
               0xd0-0xdf(?): operating systemm specific information
               0xfc: encapsulation (another opinion)
               0xfe: extended attribute - platform information(another opinion)
               0xff: extended attribute - permission, owner-id and timestamp
                     (level 3 on UNLHA32) */
            skip_bytes(header_size - n);
            break;
        }

        if (hcrc)
		{
            *hcrc = m_crcio.calccrc(*hcrc, pBuf, header_size);
		}

        if (pHeader->size_field_length == 2)
		{
            whole_size += header_size = get_word();
		}
        else
		{
            whole_size += header_size = get_longword();
		}
    }

    /* concatenate dirname and filename */
    if (pHeader->dirname.length() > 0) 
	{
		pHeader->name += pHeader->dirname;
    }

    return whole_size;
}


/*
 * level 0 header
 *
 *
 * offset  size  field name
 * ----------------------------------
 *     0      1  header size    [*1]
 *     1      1  header sum
 *            ---------------------------------------
 *     2      5  method ID                         ^
 *     7      4  packed size    [*2]               |
 *    11      4  original size                     |
 *    15      2  time                              |
 *    17      2  date                              |
 *    19      1  attribute                         | [*1] header size (X+Y+22)
 *    20      1  level (0x00 fixed)                |
 *    21      1  name length                       |
 *    22      X  pathname                          |
 * X +22      2  file crc (CRC-16)                 |
 * X +24      Y  ext-header(old style)             v
 * -------------------------------------------------
 * X+Y+24        data                              ^
 *                 :                               | [*2] packed size
 *                 :                               v
 * -------------------------------------------------
 *
 * ext-header(old style)
 *     0      1  ext-type ('U')
 *     1      1  minor version
 *     2      4  UNIX time
 *     6      2  mode
 *     8      2  uid
 *    10      2  gid
 *
 * attribute (MS-DOS)
 *    bit1  read only
 *    bit2  hidden
 *    bit3  system
 *    bit4  volume label
 *    bit5  directory
 *    bit6  archive bit (need to backup)
 *
 */
bool CLzHeader::get_header_level0(CAnsiFile &ArchiveFile, LzHeader *pHeader, CReadBuffer &Buffer)
{
    size_t header_size;
    size_t extend_size;

    pHeader->size_field_length = 2; /* in bytes */
    pHeader->header_size = header_size = get_byte();
    int checksum = get_byte();
	
	unsigned char *pBuf = Buffer.GetBegin();

    if (ArchiveFile.Read(pBuf + COMMON_HEADER_SIZE, header_size + 2 - COMMON_HEADER_SIZE) == false) 
	{
		throw ArcException("Invalid header (LHarc file ?)", checksum);
    }

    if (calc_sum(pBuf + I_METHOD, header_size) != checksum)
	{
		throw ArcException("Checksum error (LHarc file?)", checksum);
    }

    get_bytes(pHeader->method, 5, sizeof(pHeader->method));
    pHeader->packed_size = get_longword();
    pHeader->original_size = get_longword();
    pHeader->unix_last_modified_stamp = generic_to_unix_stamp(get_longword());
    pHeader->attribute = get_byte(); /* MS-DOS attribute */
    pHeader->header_level = get_byte();
    int name_length = get_byte();
    pHeader->name = get_string(name_length);

    /* defaults for other type */
    pHeader->unix_mode = UNIX_FILE_REGULAR | UNIX_RW_RW_RW;
    pHeader->unix_gid = 0;
    pHeader->unix_uid = 0;

    extend_size = header_size+2 - name_length - 24;

    if (extend_size < 0) 
	{
        if (extend_size == -2) 
		{
            /* CRC field is not given */
            pHeader->extend_type = EXTEND_GENERIC;
            pHeader->has_crc = false;
            return true;
        } 

		throw ArcException("Unknown header (lha file?)", extend_size);
    }

    pHeader->has_crc = true;
    pHeader->crc = get_word();

    if (extend_size == 0)
	{
        return true;
	}

    pHeader->extend_type = get_byte();
    extend_size--;

    if (pHeader->extend_type == EXTEND_UNIX) 
	{
        if (extend_size >= 11) 
		{
            pHeader->minor_version = get_byte();
            pHeader->unix_last_modified_stamp = (time_t) get_longword();
            pHeader->unix_mode = get_word();
            pHeader->unix_uid = get_word();
            pHeader->unix_gid = get_word();
            extend_size -= 11;
        } 
		else 
		{
            pHeader->extend_type = EXTEND_GENERIC;
        }
    }
    if (extend_size > 0)
	{
        skip_bytes(extend_size);
	}

    pHeader->header_size += 2;
    return true;
}


/*
 * level 1 header
 *
 *
 * offset   size  field name
 * -----------------------------------
 *     0       1  header size   [*1]
 *     1       1  header sum
 *             -------------------------------------
 *     2       5  method ID                        ^
 *     7       4  skip size     [*2]               |
 *    11       4  original size                    |
 *    15       2  time                             |
 *    17       2  date                             |
 *    19       1  attribute (0x20 fixed)           | [*1] header size (X+Y+25)
 *    20       1  level (0x01 fixed)               |
 *    21       1  name length                      |
 *    22       X  filename                         |
 * X+ 22       2  file crc (CRC-16)                |
 * X+ 24       1  OS ID                            |
 * X +25       Y  ???                              |
 * X+Y+25      2  next-header size                 v
 * -------------------------------------------------
 * X+Y+27      Z  ext-header                       ^
 *                 :                               |
 * -----------------------------------             | [*2] skip size
 * X+Y+Z+27       data                             |
 *                 :                               v
 * -------------------------------------------------
 *
 */
bool CLzHeader::get_header_level1(CAnsiFile &ArchiveFile, LzHeader *pHeader, CReadBuffer &Buffer)
{
    size_t header_size;
    size_t extend_size;
    int checksum;

    pHeader->size_field_length = 2; /* in bytes */
    pHeader->header_size = header_size = get_byte();
    checksum = get_byte();

	unsigned char *pBuf = Buffer.GetBegin();
	
    if (ArchiveFile.Read(pBuf + COMMON_HEADER_SIZE, header_size + 2 - COMMON_HEADER_SIZE) == false) 
	{
		throw ArcException("Invalid header (LHarc file ?)", checksum);
    }

    if (calc_sum(pBuf + I_METHOD, header_size) != checksum) 
	{
		throw ArcException("Checksum error (LHarc file?)", checksum);
    }

    get_bytes(pHeader->method, 5, sizeof(pHeader->method));
    pHeader->packed_size = get_longword(); /* skip size */
    pHeader->original_size = get_longword();
    pHeader->unix_last_modified_stamp = generic_to_unix_stamp(get_longword());
    pHeader->attribute = get_byte(); /* 0x20 fixed */
    pHeader->header_level = get_byte();

    int name_length = get_byte();
    //i = get_bytes(pHeader->name, name_length, sizeof(pHeader->name)-1);
    pHeader->name = get_string(name_length);

    /* defaults for other type */
    pHeader->unix_mode = UNIX_FILE_REGULAR | UNIX_RW_RW_RW;
    pHeader->unix_gid = 0;
    pHeader->unix_uid = 0;
    pHeader->has_crc = true;
    pHeader->crc = get_word();
    pHeader->extend_type = get_byte();

    int dummy = header_size+2 - name_length - I_LEVEL1_HEADER_SIZE;
    if (dummy > 0)
	{
        skip_bytes(dummy); /* skip old style extend header */
	}

    extend_size = get_word();
    extend_size = get_extended_header(ArchiveFile, pHeader, Buffer, extend_size, 0);
    if (extend_size == -1)
	{
        return false;
	}

    /* On level 1 header, size fields should be adjusted. */
    /* the `packed_size' field contains the extended header size. */
    /* the `header_size' field does not. */
    pHeader->packed_size -= extend_size;
    pHeader->header_size += extend_size + 2;

    return true;
}

/*
 * level 2 header
 *
 *
 * offset   size  field name
 * --------------------------------------------------
 *     0       2  total header size [*1]           ^
 *             -----------------------             |
 *     2       5  method ID                        |
 *     7       4  packed size       [*2]           |
 *    11       4  original size                    |
 *    15       4  time                             |
 *    19       1  RESERVED (0x20 fixed)            | [*1] total header size
 *    20       1  level (0x02 fixed)               |      (X+26+(1))
 *    21       2  file crc (CRC-16)                |
 *    23       1  OS ID                            |
 *    24       2  next-header size                 |
 * -----------------------------------             |
 *    26       X  ext-header                       |
 *                 :                               |
 * -----------------------------------             |
 * X +26      (1) padding                          v
 * -------------------------------------------------
 * X +26+(1)      data                             ^
 *                 :                               | [*2] packed size
 *                 :                               v
 * -------------------------------------------------
 *
 */
bool CLzHeader::get_header_level2(CAnsiFile &ArchiveFile, LzHeader *pHeader, CReadBuffer &Buffer)
{
    size_t header_size;
    size_t extend_size;

    pHeader->size_field_length = 2; /* in bytes */
    pHeader->header_size = header_size = get_word();

	unsigned char *pBuf = Buffer.GetBegin();
	
    if (ArchiveFile.Read(pBuf + COMMON_HEADER_SIZE, I_LEVEL2_HEADER_SIZE - COMMON_HEADER_SIZE) == false) 
	{
		throw ArcException("Invalid header (LHarc file ?)", 0);
    }

    get_bytes(pHeader->method, 5, sizeof(pHeader->method));
    pHeader->packed_size = get_longword();
    pHeader->original_size = get_longword();
    pHeader->unix_last_modified_stamp = get_longword();
    pHeader->attribute = get_byte(); /* reserved */
    pHeader->header_level = get_byte();

    /* defaults for other type */
    pHeader->unix_mode = UNIX_FILE_REGULAR | UNIX_RW_RW_RW;
    pHeader->unix_gid = 0;
    pHeader->unix_uid = 0;
    pHeader->has_crc = true;
    pHeader->crc = get_word();
    pHeader->extend_type = get_byte();
    extend_size = get_word();

    unsigned int hcrc = 0;
    hcrc = m_crcio.calccrc(hcrc, pBuf, (unsigned char*)m_get_ptr - pBuf);

    extend_size = get_extended_header(ArchiveFile, pHeader, Buffer, extend_size, &hcrc);
    if (extend_size == -1)
	{
        return false;
	}

    int padding = header_size - I_LEVEL2_HEADER_SIZE - extend_size;
    while (padding--)           /* padding should be 0 or 1 */
	{
        hcrc = m_crcio.UpdateCrc(hcrc, fgetc((FILE*)ArchiveFile));
	}

    if (pHeader->header_crc != hcrc)
	{
		throw ArcException("header CRC error", hcrc);
	}

    return true;
}

/*
 * level 3 header
 *
 *
 * offset   size  field name
 * --------------------------------------------------
 *     0       2  size field length (4 fixed)      ^
 *     2       5  method ID                        |
 *     7       4  packed size       [*2]           |
 *    11       4  original size                    |
 *    15       4  time                             |
 *    19       1  RESERVED (0x20 fixed)            | [*1] total header size
 *    20       1  level (0x03 fixed)               |      (X+32)
 *    21       2  file crc (CRC-16)                |
 *    23       1  OS ID                            |
 *    24       4  total header size [*1]           |
 *    28       4  next-header size                 |
 * -----------------------------------             |
 *    32       X  ext-header                       |
 *                 :                               v
 * -------------------------------------------------
 * X +32          data                             ^
 *                 :                               | [*2] packed size
 *                 :                               v
 * -------------------------------------------------
 *
 */
bool CLzHeader::get_header_level3(CAnsiFile &ArchiveFile, LzHeader *pHeader, CReadBuffer &Buffer)
{
    size_t header_size;
    size_t extend_size;

    pHeader->size_field_length = get_word();

	unsigned char *pBuf = Buffer.GetBegin();
	
    if (ArchiveFile.Read(pBuf + COMMON_HEADER_SIZE, I_LEVEL3_HEADER_SIZE - COMMON_HEADER_SIZE) == false) 
	{
		throw ArcException("Invalid header (LHarc file ?)", 0);
    }

    get_bytes(pHeader->method, 5, sizeof(pHeader->method));
    pHeader->packed_size = get_longword();
    pHeader->original_size = get_longword();
    pHeader->unix_last_modified_stamp = get_longword();
    pHeader->attribute = get_byte(); /* reserved */
    pHeader->header_level = get_byte();

    /* defaults for other type */
    pHeader->unix_mode = UNIX_FILE_REGULAR | UNIX_RW_RW_RW;
    pHeader->unix_gid = 0;
    pHeader->unix_uid = 0;
    pHeader->has_crc = true;
    pHeader->crc = get_word();
    pHeader->extend_type = get_byte();
    pHeader->header_size = header_size = get_longword();
    extend_size = get_longword();

    unsigned int hcrc = 0;
    hcrc = m_crcio.calccrc(hcrc, pBuf, (unsigned char*)m_get_ptr - pBuf);

    extend_size = get_extended_header(ArchiveFile, pHeader, Buffer, extend_size, &hcrc);
    if (extend_size == -1)
	{
        return false;
	}

    int padding = header_size - I_LEVEL3_HEADER_SIZE - extend_size;
    while (padding--)           /* padding should be 0 */
	{
        hcrc = m_crcio.UpdateCrc(hcrc, fgetc((FILE*)ArchiveFile));
	}

    if (pHeader->header_crc != hcrc)
	{
		throw ArcException("header CRC error", hcrc);
	}

    return true;
}


