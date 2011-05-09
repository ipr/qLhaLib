//////////////////////////////////
//
// structure and code for handling Lz-archive header
//
// partly rewritten from header.c in LHa for UNIX
//
// Ilkka Prusi 2011
//
// original copyrights:
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


#include "LhHeader.h"

#include <QString>


void CLhHeader::ParseHeaders(CAnsiFile &ArchiveFile)
{
    //char *archive_delim = "\377\\"; /* `\' is for level 0 header and broken archive. */
	
	if (m_pReadBuffer == nullptr)
	{
		// emulate old style, read 4096 max. at a time
		m_pReadBuffer = new CReadBuffer(4096); // default bufsize
	}
	m_pReadBuffer->PrepareBuffer(4096, false);

	bool bIsEnd = false;
	while (bIsEnd == false)
	{
		m_get_ptr = (char*)m_pReadBuffer->GetBegin();
		m_get_ptr_end = (char*)m_pReadBuffer->GetEnd();
		
		int end_mark = getc((FILE*)ArchiveFile);
		if (end_mark == EOF || end_mark == 0) 
		{
			bIsEnd = true;
			break;           /* finish */
		}
		m_get_ptr[0] = end_mark;
		
		if (ArchiveFile.Read(m_get_ptr + 1, COMMON_HEADER_SIZE - 1) == false) 
		{
			throw ArcException("Invalid header (LHarc file ?)", "");
		}
		
		LzHeader *pHeader = new LzHeader();
		m_HeaderList.push_back(pHeader);
		
		bool bRet = false;
		switch (m_get_ptr[I_HEADER_LEVEL]) 
		{
		case 0:
			bRet = get_header_level0(ArchiveFile, pHeader);
			break;
		case 1:
			bRet = get_header_level1(ArchiveFile, pHeader);
			break;
		case 2:
			bRet = get_header_level2(ArchiveFile, pHeader);
			break;
		case 3:
			bRet = get_header_level3(ArchiveFile, pHeader);
			break;
		default:
			throw ArcException("Unknown level header", m_get_ptr[I_HEADER_LEVEL]);
		}
		
		if (bRet == false)
		{
			throw ArcException("Failure reading header", "");
		}
		
		// keep this file offset (after header)
		// to locate data in archive for the file-entry later
		if (ArchiveFile.Tell(pHeader->data_pos) == false)
		{
			throw IOException("Failure getting current position");
		}
		
		// fix path-names
		pHeader->filename.replace('\\', "/");
		pHeader->dirname.replace('\\', "/");
		
		if (pHeader->UnixMode.IsSymLink() == true) 
		{
			/* hdr->name is symbolic link name */
			/* hdr->realname is real name */
			int iPos = pHeader->filename.lastIndexOf('|');
			if (iPos == -1)
			{
				// not found as expecing -> fatal
				throw ArcException("Unknown symlink name", pHeader->filename.toStdString());
			}
			pHeader->realname = pHeader->filename.left(iPos +1);
		}
		
		// seek past actual data of the entry in archive
		if (ArchiveFile.Seek(pHeader->packed_size, SEEK_CUR) == false)
		{
			throw IOException("Failure seeking next header");
		}
		
		emit message(QString("Header found: %1").arg(pHeader->filename));
	}
}


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
size_t CLhHeader::get_extended_header(CAnsiFile &ArchiveFile, LzHeader *pHeader, size_t header_size, unsigned int *hcrc)
{
    size_t whole_size = header_size;
    int n = 1 + pHeader->size_field_length; /* `ext-type' + `next-header size' */

    if (pHeader->header_level == 0)
	{
        return 0;
	}

	// clear or allocate larger if necessary
	m_pReadBuffer->PrepareBuffer(header_size, false);
	
    while (header_size) 
	{
		m_get_ptr = (char*)m_pReadBuffer->GetBegin();
		
		// this only makes sense for "wrap-under" case of unsigned counter..
        if (m_pReadBuffer->GetSize() < header_size) 
		{
			throw ArcException("header size too large.", header_size);
        }

        if (ArchiveFile.Read(m_pReadBuffer->GetBegin(), header_size) == false) 
		{
			throw ArcException("Invalid header (LHa file ?)", 0);
        }

		unsigned char *pBuf = m_pReadBuffer->GetBegin();
		
		// TODO:
        //tExtendedAttribs enExtType = (tExtendedAttribs)get_byte();
        int iExtType = get_byte();
        switch (iExtType) 
		{
        case EXTH_CRC:
            /* header crc (CRC-16) */
            pHeader->header_crc = get_word();
            /* clear buffer for CRC calculation. */
            pBuf[1] = pBuf[2] = 0;
            skip_bytes(header_size - n - 2);
            break;
        case EXTH_FILENAME:
            /* filename */
            //name_length = get_bytes(pHeader->name, header_size-n, sizeof(pHeader->name)-1);
            pHeader->filename = get_string(header_size-n);
            break;
        case EXTH_PATH:
            /* directory */
            //dir_length = get_bytes(dirname, header_size-n, sizeof(dirname)-1);
            pHeader->dirname = get_string(header_size-n);
            break;
			
        case EXTH_MSDOSATTRIBS:
            /* MS-DOS attribute flags */
            pHeader->MsDosAttributes.SetFromValue(get_word());
            break;
			
        case EXTH_WINDOWSTIMES:
			{
				/* Windows time stamp (FILETIME structure) */
				/* time in 100 nanosecond-intervals since 1601-01-01 00:00:00 (UTC) */
				
				CFiletimeHelper ftCreation = get_wintime();
				CFiletimeHelper ftLastModified = get_wintime();
				CFiletimeHelper ftLastAccess = get_wintime();
				
				pHeader->creation_stamp.setTime_t((time_t)ftCreation);
				pHeader->last_modified_stamp.setTime_t((time_t)ftLastModified); // already set ?
				pHeader->last_access_stamp.setTime_t((time_t)ftLastAccess);
				
				// last modified time
				//if (pHeader->header_level >= 2)  /* time_t has been already set */
			}
			break;
        case EXTH_UNIXPERMISSIONS:
            /* UNIX permission */
            pHeader->UnixMode.unix_mode = get_word();
            break;
        case EXTH_UNIXGIDUID:
            /* UNIX gid and uid */
            pHeader->unix_gid = get_word();
            pHeader->unix_uid = get_word();
            break;
        case EXTH_UNIXGROUP:
            /* UNIX group name */
            //i = get_bytes(pHeader->group, header_size-n, sizeof(pHeader->group)-1);
            pHeader->group = get_string(header_size-n);
            break;
        case EXTH_UNIXUSER:
            /* UNIX user name */
            //i = get_bytes(pHeader->user, header_size-n, sizeof(pHeader->user)-1);
            pHeader->user = get_string(header_size-n);
            break;
        case EXTH_UNIXLASTMODIFIED:
            /* UNIX last modified time */
			// (32-bit time_t)
            pHeader->last_modified_stamp.setTime_t((time_t)get_longword());
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
			emit warning(QString("unknown extended header %1").arg(iExtType));
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
		// we assume there is a path separator..
		pHeader->filename.insert(0, pHeader->dirname);
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
bool CLhHeader::get_header_level0(CAnsiFile &ArchiveFile, LzHeader *pHeader)
{
    size_t header_size;
    size_t extend_size;
	int checksum = 0;

    pHeader->size_field_length = 2; /* in bytes */
    pHeader->header_size = header_size = get_byte();
    checksum = get_byte();
	
	unsigned char *pBuf = m_pReadBuffer->GetBegin();

    if (ArchiveFile.Read(pBuf + COMMON_HEADER_SIZE, header_size + 2 - COMMON_HEADER_SIZE) == false) 
	{
		throw ArcException("Invalid header (LHarc file ?)", checksum);
    }

    if (calc_sum(pBuf + I_METHOD, header_size) != checksum)
	{
		throw ArcException("Checksum error (LHarc file?)", checksum);
    }

	// there's size to it given so use it
    get_bytes(pHeader->method, METHOD_TYPE_STORAGE, METHOD_TYPE_STORAGE);
    pHeader->packed_size = get_longword();
    pHeader->original_size = get_longword();
	CGenericTime gtStamp(get_longword());
    pHeader->last_modified_stamp.setTime_t((time_t)gtStamp);
    pHeader->MsDosAttributes.SetFromValue(get_byte()); /* MS-DOS attribute */
    pHeader->header_level = get_byte();
    int name_length = get_byte();
    pHeader->filename = get_string(name_length);

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
            pHeader->last_modified_stamp.setTime_t((time_t)get_longword());
            pHeader->UnixMode.unix_mode = get_word();
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
bool CLhHeader::get_header_level1(CAnsiFile &ArchiveFile, LzHeader *pHeader)
{
    size_t header_size;
	int checksum = 0;

    pHeader->size_field_length = 2; /* in bytes */
    pHeader->header_size = header_size = get_byte();
	checksum = get_byte();

	unsigned char *pBuf = m_pReadBuffer->GetBegin();
	
    if (ArchiveFile.Read(pBuf + COMMON_HEADER_SIZE, header_size + 2 - COMMON_HEADER_SIZE) == false) 
	{
		throw ArcException("Invalid header (LHarc file ?)", checksum);
    }

    if (calc_sum(pBuf + I_METHOD, header_size) != checksum) 
	{
		throw ArcException("Checksum error (LHarc file?)", checksum);
    }

	// there's size to it given so use it
    get_bytes(pHeader->method, METHOD_TYPE_STORAGE, METHOD_TYPE_STORAGE);
    pHeader->packed_size = get_longword(); /* skip size */
    pHeader->original_size = get_longword();
	CGenericTime gtStamp(get_longword());
    pHeader->last_modified_stamp.setTime_t((time_t)gtStamp);
    pHeader->MsDosAttributes.SetFromValue(get_byte()); /* 0x20 fixed */
    pHeader->header_level = get_byte();

    int name_length = get_byte();
    //i = get_bytes(pHeader->name, name_length, sizeof(pHeader->name)-1);
    pHeader->filename = get_string(name_length);

    /* defaults for other type */
    pHeader->has_crc = true;
    pHeader->crc = get_word();
    pHeader->extend_type = get_byte();

    int dummy = header_size+2 - name_length - I_LEVEL1_HEADER_SIZE;
    if (dummy > 0)
	{
        skip_bytes(dummy); /* skip old style extend header */
	}

    size_t extend_size = get_word();
    extend_size = get_extended_header(ArchiveFile, pHeader, extend_size, 0);
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
bool CLhHeader::get_header_level2(CAnsiFile &ArchiveFile, LzHeader *pHeader)
{
    size_t header_size;

    pHeader->size_field_length = 2; /* in bytes */
    pHeader->header_size = header_size = get_word();

	unsigned char *pBuf = m_pReadBuffer->GetBegin();
	
    if (ArchiveFile.Read(pBuf + COMMON_HEADER_SIZE, I_LEVEL2_HEADER_SIZE - COMMON_HEADER_SIZE) == false) 
	{
		throw ArcException("Invalid header (LHarc file ?)", 0);
    }

	// there's size to it given so use it
    get_bytes(pHeader->method, METHOD_TYPE_STORAGE, METHOD_TYPE_STORAGE);
    pHeader->packed_size = get_longword();
    pHeader->original_size = get_longword();
    pHeader->last_modified_stamp.setTime_t((time_t)get_longword());
    pHeader->MsDosAttributes.SetFromValue(get_byte()); /* reserved */
    pHeader->header_level = get_byte();

    /* defaults for other type */
    pHeader->has_crc = true;
    pHeader->crc = get_word();
    pHeader->extend_type = get_byte();
	
    size_t extend_size = get_word();

    unsigned int hcrc = 0;
    hcrc = m_crcio.calccrc(hcrc, pBuf, (unsigned char*)m_get_ptr - pBuf);

    extend_size = get_extended_header(ArchiveFile, pHeader, extend_size, &hcrc);
    if (extend_size == -1)
	{
        return false;
	}

    int padding = header_size - I_LEVEL2_HEADER_SIZE - extend_size;
	UpdatePaddingToCrc(ArchiveFile, hcrc, padding);

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
bool CLhHeader::get_header_level3(CAnsiFile &ArchiveFile, LzHeader *pHeader)
{
    pHeader->size_field_length = get_word();

	unsigned char *pBuf = m_pReadBuffer->GetBegin();
	
    if (ArchiveFile.Read(pBuf + COMMON_HEADER_SIZE, I_LEVEL3_HEADER_SIZE - COMMON_HEADER_SIZE) == false) 
	{
		throw ArcException("Invalid header (LHarc file ?)", 0);
    }

	// there's size to it given so use it
    get_bytes(pHeader->method, METHOD_TYPE_STORAGE, METHOD_TYPE_STORAGE);
    pHeader->packed_size = get_longword();
    pHeader->original_size = get_longword();
    pHeader->last_modified_stamp.setTime_t((time_t)get_longword());
    pHeader->MsDosAttributes.SetFromValue(get_byte()); /* reserved */
    pHeader->header_level = get_byte();

    /* defaults for other type */
    pHeader->has_crc = true;
    pHeader->crc = get_word();
    pHeader->extend_type = get_byte();
    //pHeader->header_size = header_size = get_longword();
    pHeader->header_size = get_longword();
	
    size_t header_size = pHeader->header_size;
    size_t extend_size = get_longword();

    unsigned int hcrc = 0;
    hcrc = m_crcio.calccrc(hcrc, pBuf, (unsigned char*)m_get_ptr - pBuf);

    extend_size = get_extended_header(ArchiveFile, pHeader, extend_size, &hcrc);
    if (extend_size == -1)
	{
        return false;
	}

    int padding = header_size - I_LEVEL3_HEADER_SIZE - extend_size;
	UpdatePaddingToCrc(ArchiveFile, hcrc, padding);

    if (pHeader->header_crc != hcrc)
	{
		throw ArcException("header CRC error", hcrc);
	}

    return true;
}

void CLhHeader::UpdatePaddingToCrc(CAnsiFile &ArchiveFile, unsigned int &hcrc, const long lPadSize)
{
	// allocate enough for padding (zeroed)
	// or just clear existing if enough
	m_pReadBuffer->PrepareBuffer(lPadSize, false);

	// check how much of file remains
	long lPos = 0;
	ArchiveFile.Tell(lPos);
	long lRemaining = (ArchiveFile.GetSize() - lPos);

	// read padding
	bool bRet = false;
	if (lRemaining < lPadSize)
	{
		bRet = ArchiveFile.Read(m_pReadBuffer->GetBegin(), lRemaining);
	}
	else
	{
		bRet = ArchiveFile.Read(m_pReadBuffer->GetBegin(), lPadSize);
	}
	
	if (bRet == false)
	{
		throw IOException("Failed reading header padding for crc");
	}

	// update crc by padding-values
	unsigned char *pData = m_pReadBuffer->GetBegin();
	for (long l = 0; l < lPadSize; l++)
	{
        hcrc = m_crcio.UpdateCrc(hcrc, pData[l]);
	}
}


