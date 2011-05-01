#include "LhArchive.h"

#include "qlhalib.h"

#include "AnsiFile.h"

#include <QTextCodec>


CLhArchive::CLhArchive(QLhALib *pParent)
	: QObject(pParent),
	m_szCurrentArchive(),
	m_nArchiveFileSize(0),
	m_ulPackedSizeTotal(0),
	m_ulUnpackedSizeTotal(0),
	m_ulFileCountTotal(0),
	m_FileHeader(),
	m_FileList()
{
}

CLhArchive::~CLhArchive(void)
{
	auto it = m_FileList.begin();
	auto itEnd = m_FileList.end();
	while (it != itEnd)
	{
		LzHeader *pHeader = (*it);
		delete pHeader;
		
		++it;
	}
	m_FileList.clear();
}

bool CLhArchive::ConvertFromCodepage(QTextCodec *pCodec)
{
	/*
	auto it = m_FileList.begin();
	auto itEnd = m_FileList.end();
	while (it != itEnd)
	{
		LzHeader *pHeader = (*it);
		
		pHeader->name = pCodec->toUnicode(pHeader->name);
		pHeader->dirname = pCodec->toUnicode(pHeader->dirname);
		pHeader->realname = pCodec->toUnicode(pHeader->realname);
		
		pHeader->user = pCodec->toUnicode(pHeader->user);
		pHeader->group = pCodec->toUnicode(pHeader->group);
		
		++it;
	}
	*/

	return true;
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
	
	if (m_FileHeader.IsValidLha(Buffer) == false)
	{
		throw ArcException("No supported header in file", m_szCurrentArchive.toStdString());
	}
	if (m_FileHeader.ParseBuffer(Buffer) == false)
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

void CLhArchive::SetArchiveFile(QString szArchive)
{
	if (m_szCurrentArchive != szArchive)
	{
		m_szCurrentArchive = szArchive;
		
		/*
		CAnsiFile ArchiveFile;
		if (ArchiveFile.Open(m_szCurrentArchive.toStdString()) == false)
		{
			// throw exception?
			// (does it affect signal handling in Qt?)
		}
		m_nArchiveFileSize = ArchiveFile.GetSize();
		*/

		// TODO:
		// close old
		// open new
	}
}

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
	CReadBuffer Buffer(4096);

	LzHeader *pHeader = nullptr;
	do
	{
		pHeader = m_FileHeader.GetNextHeader(Buffer, ArchiveFile);
		if (pHeader != nullptr)
		{
			// -> add file entry of it to list
			m_FileList.push_back(pHeader);
		}
	} while (pHeader != nullptr);
	
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
