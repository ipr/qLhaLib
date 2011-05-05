#include "LhArchive.h"

#include "qlhalib.h"

#include "AnsiFile.h"

#include <QTextCodec>


CLhArchive::CLhArchive(QLhALib *pParent, QString &szArchive)
	: QObject(pParent),
	m_szCurrentArchive(szArchive),
	m_nArchiveFileSize(0),
	m_ulPackedSizeTotal(0),
	m_ulUnpackedSizeTotal(0),
	m_ulFileCountTotal(0),
	m_pHeaders(nullptr)
{
	m_pHeaders = new CLhHeader(this);
	
	//connect(m_pHeaders, SIGNAL(message(QString)), this, SIGNAL(message(QString)));
	//connect(m_pHeaders, SIGNAL(warning(QString)), this, SIGNAL(warning(QString)));
	//connect(m_pHeaders, SIGNAL(FileLocated(LzHeader*)), this, SLOT(FileLocated(LzHeader*)));
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
		throw ArcException("No supported header in file", m_szCurrentArchive.toStdString());
	}
	if (m_pHeaders->ParseBuffer(Buffer) == false)
	{
		throw ArcException("Failed to parse header", m_szCurrentArchive.toStdString());
	}
	
	// just seek start.. 
	if (ArchiveFile.Seek(0, SEEK_SET) == false)
	{
		throw IOException("Failed seeking start");
	}
}


/////////////// public slots

/*
void CLhArchive::SetArchiveFile(QString szArchive)
{
	m_szCurrentArchive = szArchive;
}
*/

void CLhArchive::SetConversionCodec(QTextCodec *pCodec)
{
	m_pHeaders->SetConversionCodec(pCodec);
}

/*
void CLhArchive::FileLocated(LzHeader *pHeader)
{
	// TEMP!
	// TESTING ONLY!
	emit message(pHeader->name);
}
*/

bool CLhArchive::Extract(QString &szExtractPath)
{
	return false;
}

bool CLhArchive::List()
{
	CAnsiFile ArchiveFile;
	if (ArchiveFile.Open(m_szCurrentArchive.toStdString()) == false)
	{
		throw IOException("Failed opening archive");
		
		// alternate:
		//emit fatal_error("Failed opening archive");
		//return false;
	}
	m_nArchiveFileSize = ArchiveFile.GetSize();

	// should have at least 16 bytes for a header to exist..
	//
	if (m_nArchiveFileSize < 16)
	{
		throw ArcException("File too small", m_szCurrentArchive.toStdString());
	}
	
	// throws exception on failure:
	// when no valid header or such
	SeekHeader(ArchiveFile);

	// emulate old style, read 4096 max. at a time
	//CReadBuffer Buffer(4096);

	// throws exception on error
	m_pHeaders->ParseHeaders(ArchiveFile);
	
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
