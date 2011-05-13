#include "LhArchive.h"


CLhArchive::CLhArchive(QLhALib *pParent, QString &szArchive)
	: QObject(pParent),
	m_szArchive(szArchive),
	m_nFileSize(0),
	m_ulTotalPacked(0),
	m_ulTotalUnpacked(0),
	m_ulTotalFiles(0),
	m_uiCrc(0),
	m_pHeaders(nullptr),
	m_pExtraction(nullptr)
{
	m_pHeaders = new CLhHeader(this);
	m_pExtraction = new CLhExtract(this);
	
	connect(m_pHeaders, SIGNAL(message(QString)), this, SIGNAL(message(QString)));
	connect(m_pHeaders, SIGNAL(warning(QString)), this, SIGNAL(warning(QString)));
	
	connect(m_pExtraction, SIGNAL(message(QString)), this, SIGNAL(message(QString)));
	connect(m_pExtraction, SIGNAL(warning(QString)), this, SIGNAL(warning(QString)));
}

CLhArchive::~CLhArchive(void)
{
	// TODO: 
	// don't destroy that which is destroyed by parent?
	
	if (m_pExtraction != nullptr)
	{
		delete m_pExtraction;
	}
	if (m_pHeaders != nullptr)
	{
		delete m_pHeaders;
	}
}

/////////////// protected methods

// in case same instance is used again (same or different archive)
// -> clear some added data
void CLhArchive::Clear()
{
	// same instance, called again?
	// (TODO: add better checks if info exists and is correct..)
	m_pHeaders->Clear();
	m_ulTotalPacked = 0;
	m_ulTotalUnpacked = 0;
	m_ulTotalFiles = 0;
	m_uiCrc = 0;
}

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

void CLhArchive::SeekContents(CAnsiFile &ArchiveFile)
{
	if (ArchiveFile.Open(m_szArchive.toStdString()) == false)
	{
		throw IOException("Failed opening archive");
	}
	m_nFileSize = ArchiveFile.GetSize();

	// only seek if not listed already
	if (m_nFileSize == 0 && m_pHeaders->m_HeaderList.size() == 0)
	{
		// check&parse archive info so we know 
		// how to extract files..
		//
		SeekHeader(ArchiveFile);

		// list file-headers (contents)
		// from the archive-file
		//
		m_pHeaders->ParseHeaders(ArchiveFile);
	}
}


/////////////// public slots

void CLhArchive::SetConversionCodec(QTextCodec *pCodec)
{
	m_pHeaders->SetConversionCodec(pCodec);
}

bool CLhArchive::Extract(QString &szExtractPath)
{
	// lookup each entry of file
	CAnsiFile ArchiveFile;

	// open and list contents
	SeekContents(ArchiveFile);

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

		emit message(QString("Extracting.. ").append(pHeader->filename));
		
		// this should have proper path-ending already..
		QString szTempPath = szExtractPath;
		szTempPath += pHeader->filename;
		
		// if it's directory-entry -> nothing more to do here
		// (usually has -lhd- compression method for "store only"?)
		if (pHeader->UnixMode.IsDirectory() == true)
		{
			// make directory only
			CPathHelper::MakePath(szTempPath.toStdString());
			
			++it;
			continue;
		}
		
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
		m_pExtraction->ExtractFile(ArchiveFile, pHeader, OutFile);
		
		++it;
	}
	
	return true;
}

// extract single file from archive to user-buffer
bool CLhArchive::ExtractToCallerBuffer(QString &szFileEntry, QByteArray &outArray)
{
	// lookup each entry of file
	CAnsiFile ArchiveFile;
	
	// only seek if not listed already
	if (m_nFileSize == 0 && m_pHeaders->m_HeaderList.size() == 0)
	{
		SeekContents(ArchiveFile);
	}
	
	auto it = m_pHeaders->m_HeaderList.begin();
	auto itEnd = m_pHeaders->m_HeaderList.end();
	while (it != itEnd)
	{
		LzHeader *pHeader = (*it);
		
		if (pHeader->UnixMode.IsDirectory() == true)
		{
			if (pHeader->filename == szFileEntry)
			{
				emit warning(QString("Wanted file %1 is a directory").arg(szFileEntry));
				return false;
			}
			++it;
			continue;
		}

		/*
		if (pHeader->filename == szFileEntry)
		{
			// extract to given buffer only
			return ExtractToBuffer(ArchiveFile, pHeader, outArray);
		}
		*/
		
		++it;
	}
	
	// throw exception instead? (user-input was crap -> not our fault)
	emit warning(QString("file %1 was not found").arg(szFileEntry));
	return false;
}

bool CLhArchive::List(QLhALib::tArchiveEntryList &lstArchiveInfo)
{
	// same instance, called again
	Clear();

	// auto-close file (on leaving scope),
	// wrap some handling
	CAnsiFile ArchiveFile;
	
	// lookup each entry of file,
	// throws exception on error
	SeekContents(ArchiveFile);
	
	// information to caller
	auto it = m_pHeaders->m_HeaderList.begin();
	auto itEnd = m_pHeaders->m_HeaderList.end();
	while (it != itEnd)
	{
		LzHeader *pHeader = (*it);
		
		// if it's directory-entry -> nothing more to do here
		if (pHeader->UnixMode.IsDirectory() == true)
		{
			++it;
			continue;
		}
		
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
