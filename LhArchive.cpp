#include "LhArchive.h"


CLhArchive::CLhArchive(QLhALib *pParent, QString &szArchive)
	: QObject(pParent),
	m_szArchive(szArchive),
	m_nFileSize(0),
	m_ulTotalPacked(0),
	m_ulTotalUnpacked(0),
	m_ulTotalFiles(0),
	m_uiCrc(0),
	m_pHeaders(nullptr)
{
	m_pHeaders = new CLhHeader(this);
}

CLhArchive::~CLhArchive(void)
{
	if (m_pHeaders != nullptr)
	{
		delete m_pHeaders;
	}
}

/////////////// protected methods

void CLhArchive::SeekHeader(CAnsiFile &ArchiveFile)
{
	//size_t nMaxSeekSize = 64 * 1024; // max seek size
	size_t nMaxSeekSize = 4096; // max seek size
	size_t nFileSize = ArchiveFile.GetSize();

	// should have at least 16 bytes for a header to exist..
	//
	if (m_nFileSize < 16)
	{
		throw ArcException("File too small", m_szArchive.toStdString());
	}

	// check if we have smaller file than expected
	// -> don't try reading more than we have
	if (nFileSize < nMaxSeekSize)
	{
		nMaxSeekSize = nFileSize;
	}
	
	CReadBuffer Buffer(nMaxSeekSize);
	if (ArchiveFile.Read(Buffer) == false)
	{
		throw IOException("Failed reading header");
	}
	
	if (m_pHeaders->IsValidLha(Buffer) == false)
	{
		throw ArcException("No supported header in file", m_szArchive.toStdString());
	}
	if (m_pHeaders->ParseBuffer(Buffer) == false)
	{
		throw ArcException("Failed to parse header", m_szArchive.toStdString());
	}
	
	// just seek start.. 
	if (ArchiveFile.Seek(0, SEEK_SET) == false)
	{
		throw IOException("Failed seeking start");
	}
}

tHuffBits CLhArchive::GetDictionaryBits(const tCompressionMethod enMethod) const
{
    switch (enMethod) 
	{
    case LZHUFF0_METHOD_NUM:    /* -lh0- */
        return LZHUFF0_DICBIT;
    case LZHUFF1_METHOD_NUM:    /* -lh1- */
        return LZHUFF1_DICBIT;
    case LZHUFF2_METHOD_NUM:    /* -lh2- */
        return LZHUFF2_DICBIT;
    case LZHUFF3_METHOD_NUM:    /* -lh2- */
        return LZHUFF3_DICBIT;
    case LZHUFF4_METHOD_NUM:    /* -lh4- */
        return LZHUFF4_DICBIT;
    case LZHUFF5_METHOD_NUM:    /* -lh5- */
        return LZHUFF5_DICBIT;
    case LZHUFF6_METHOD_NUM:    /* -lh6- */
        return LZHUFF6_DICBIT;
    case LZHUFF7_METHOD_NUM:    /* -lh7- */
        return LZHUFF7_DICBIT;
    case LARC_METHOD_NUM:       /* -lzs- */
        return LARC_DICBIT;
    case LARC5_METHOD_NUM:      /* -lz5- */
        return LARC5_DICBIT;
    case LARC4_METHOD_NUM:      /* -lz4- */
        return LARC4_DICBIT;
		
    default:
		break;
    }

	//warning("unknown method %d", method);
	return LZHUFF5_DICBIT; /* for backward compatibility */
}

// decode data from archive and write to prepared output file,
// use given metadata as help..
//
void CLhArchive::ExtractFile(CAnsiFile &ArchiveFile, LzHeader *pHeader, CAnsiFile &OutFile)
{
	// TODO: determine decoding method
	//if (pHeader->method == ???)
	// -> simplify, make enumerated type
	tCompressionMethod enMethod = pHeader->GetMethod();
	if (enMethod == LZHDIRS_METHOD_NUM)
	{
		// just make directories and no actual files ?
		return;
	}

    size_t read_size = 0;
	unsigned int crc = 0;
	
	// huffman dictionary bits
	tHuffBits enHuffBits = GetDictionaryBits(enMethod);
	if ((int)enHuffBits == 0)
	{
		// no compression, just copy to output
		
		CReadBuffer Buf(4096);
		size_t nToWrite = pHeader->original_size;
		while (nToWrite > 0)
		{
			read_size = nToWrite;
			if (read_size > 4096)
			{
				read_size = 4096;
			}
			
			if (ArchiveFile.Read(Buf.GetBegin(), read_size) == false)
			{
				throw IOException("Failed reading data");
			}
			
			// update crc (could make static instance..)
            crc = m_pHeaders->m_crcio.calccrc(crc, Buf.GetBegin(), read_size);
			
			// write output
			if (OutFile.Write(Buf.GetBegin(), read_size) == false)
			{
				throw IOException("Failed writing output");
			}
			nToWrite -= read_size;
		}
		
		// file done
	}
	else
	{
		// LZ decoding
		//crc = decode_output();
	}
	
	// flush to disk
	OutFile.Flush();
	
	// verify CRC
	if (pHeader->has_crc == true && pHeader->crc != crc)
	{
		//throw ArcException("CRC error on extract", pHeader->filename.toStdString());
	}
}


/////////////// public slots

void CLhArchive::SetConversionCodec(QTextCodec *pCodec)
{
	m_pHeaders->SetConversionCodec(pCodec);
}

// TODO: also support for specific files to extract?
//bool CLhArchive::Extract(QString &szExtractPath, QStringList &lstFiles)

bool CLhArchive::Extract(QString &szExtractPath)
{
	// same instance, called again?
	// (TODO: add better checks if info exists and is correct..)
	m_pHeaders->Clear();

	CAnsiFile ArchiveFile;
	if (ArchiveFile.Open(m_szArchive.toStdString()) == false)
	{
		throw IOException("Failed opening archive");
	}
	m_nFileSize = ArchiveFile.GetSize();

	// check&parse archive info so we know 
	// how to extract files..
	//
	SeekHeader(ArchiveFile);
	m_pHeaders->ParseHeaders(ArchiveFile);

	// make user-given path where to extract (may be empty)
	CPathHelper::MakePath(szExtractPath.toStdString());

	// note: archive-wide crc counting also ?
	// (see member)
    //unsigned int crc;
	
	// decode and write each file from archive
	//
	auto it = m_pHeaders->m_HeaderList.begin();
	auto itEnd = m_pHeaders->m_HeaderList.end();
	while (it != itEnd)
	{
		LzHeader *pHeader = (*it);

		// TODO: if only specific files are extracted,
		// check if this is listed (note: paths..)
		/*
		if (lstFiles.contains(pHeader->filename) == false)
		{
			++it;
			continue;
		}
		*/
		
		// this should have proper path-ending already..
		QString szTempPath = szExtractPath;
		szTempPath += pHeader->filename;
		
		// create path to file
		CPathHelper::MakePathToFile(szTempPath.toStdString());
		CAnsiFile OutFile;
		if (OutFile.Open(szTempPath.toStdString(), true) == false)
		{
			throw ArcException("Failed creating file for writing", szTempPath.toStdString());
		}
		
		// seek in archive where data for this entry begins..
		if (ArchiveFile.Seek(pHeader->data_pos, SEEK_SET) == false)
		{
			throw IOException("Failure seeking entry data");
		}
		
		// decode from archive to output..
		// give parsed metadata and prepared output also
		//
		ExtractFile(ArchiveFile, pHeader, OutFile);
		
		++it;
	}
	
	return true;
}

bool CLhArchive::List(QLhALib::tArchiveEntryList &lstArchiveInfo)
{
	// same instance, called again?
	// (TODO: add better checks if info exists and is correct..)
	m_pHeaders->Clear();
	
	// auto-close file (on leaving scope),
	// wrap some handling
	CAnsiFile ArchiveFile;
	if (ArchiveFile.Open(m_szArchive.toStdString()) == false)
	{
		throw IOException("Failed opening archive");
	}
	m_nFileSize = ArchiveFile.GetSize();

	// throws exception on failure:
	// when no valid header or such
	SeekHeader(ArchiveFile);

	// throws exception on error
	m_pHeaders->ParseHeaders(ArchiveFile);

	// information to caller
	auto it = m_pHeaders->m_HeaderList.begin();
	auto itEnd = m_pHeaders->m_HeaderList.end();
	while (it != itEnd)
	{
		LzHeader *pHeader = (*it);
		
		lstArchiveInfo.push_back(QLhALib::CArchiveEntry());
		
		QLhALib::CArchiveEntry &Entry = lstArchiveInfo.back();
		Entry.m_szFileName = pHeader->filename;
		Entry.m_uiCrc = pHeader->crc;
		Entry.m_ulPackedSize = pHeader->packed_size;
		Entry.m_ulUnpackedSize = pHeader->original_size;
		Entry.m_szPackMode = QString::fromAscii(pHeader->method, METHOD_TYPE_STORAGE);
		Entry.m_Stamp = pHeader->last_modified_stamp;
		
		// attributes? which ones?
		// unix/msdos? not always both..
		
		// update archive statistics
		m_ulTotalPacked += pHeader->packed_size;
		m_ulTotalUnpacked += pHeader->original_size;
		m_ulTotalFiles++;
		
		++it;
	}
	
	return true;
}

bool CLhArchive::Test()
{
	return false;
}

bool CLhArchive::AddFiles(QStringList &lstFiles)
{
	CAnsiFile ArchiveFile;
	if (ArchiveFile.Open(m_szArchive.toStdString(), true) == false)
	{
		throw IOException("Failed opening archive for writing");
	}
	
	if (ArchiveFile.GetSize() > 0)
	{
		// adding to existing archive
		// -> check existing data
		
		//SeekHeader(ArchiveFile);
		//m_pHeaders->ParseHeaders(ArchiveFile);
	}
	else
	{
		// write archive header to new file
	}
	
	// now add specified files to archive..

	
	return false;
}
