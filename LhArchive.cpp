#include "LhArchive.h"


CLhArchive::CLhArchive(QLhALib *pParent, QString &szArchive)
	: QObject(pParent),
	m_szArchive(szArchive),
	m_nFileSize(0),
	m_ulTotalPacked(0),
	m_ulTotalUnpacked(0),
	m_ulTotalFiles(0),
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


/////////////// public slots

void CLhArchive::SetConversionCodec(QTextCodec *pCodec)
{
	m_pHeaders->SetConversionCodec(pCodec);
}

bool CLhArchive::Extract(QString &szExtractPath)
{
	return false;
}

bool CLhArchive::List(QLhALib::tArchiveEntryList &lstArchiveInfo)
{
	// auto-close file (on leaving scope),
	// wrap some handling
	CAnsiFile ArchiveFile;
	if (ArchiveFile.Open(m_szArchive.toStdString()) == false)
	{
		throw IOException("Failed opening archive");
	}
	m_nFileSize = ArchiveFile.GetSize();

	// should have at least 16 bytes for a header to exist..
	//
	if (m_nFileSize < 16)
	{
		throw ArcException("File too small", m_szArchive.toStdString());
	}
	
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
	return false;
}
