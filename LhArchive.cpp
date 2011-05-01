#include "LhArchive.h"

#include "qlhalib.h"

#include "AnsiFile.h"


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

/////////////// protected methods

void CLhArchive::SeekHeader(CAnsiFile &ArchiveFile)
{
	size_t nMaxSeekSize = 64 * 1024; // max seek size
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
		throw ArcException("No supported header in file", m_szCurrentArchive);
	}
	if (m_FileHeader.ParseBuffer(Buffer) == false)
	{
		throw ArcException("Failed to parse header", m_szCurrentArchive);
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
		throw ArcException("File too small", m_szCurrentArchive);
	}
	
	// throws exception on failure:
	// when no valid header or such
	SeekHeader(ArchiveFile);

	// just read it at once and be done with it..
	// 
	CReadBuffer Buffer(m_nArchiveFileSize);
	if (ArchiveFile.Read(Buffer) == false)
	{
		throw IOException("Failed reading archive");
	}
	
	// just seek start.. 
	if (ArchiveFile.Seek(0, SEEK_SET) == false)
	{
		throw IOException("Failed seeking start");
	}
	
	LzHeader *pHeader = nullptr;
	do
	{
		pHeader = m_FileHeader.GetNextHeader(Buffer, ArchiveFile);
		if (pHeader != nullptr)
		{
			// -> add file entry of it to list
			delete pHeader;
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
