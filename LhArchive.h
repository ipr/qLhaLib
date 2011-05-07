//////////////////////////////////////////////
//
// code to handle LHA archive-file "commands":
// - list
// - extract
// - add/update
//
// Ilkka Prusi 2011
//

#ifndef LHARCHIVE_H
#define LHARCHIVE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTextCodec>

// use typedefs from parent
#include <qlhalib.h>

#include "AnsiFile.h"
#include "LhaTypeDefs.h"
#include "LhHeader.h"



class CLhArchive : public QObject
{
	Q_OBJECT

public:
    CLhArchive(QLhALib *pParent, QString &szArchive);
	virtual ~CLhArchive(void);
	
protected:
	QString m_szArchive;
	size_t m_nFileSize;
	
	// some statistics of archive
	unsigned long m_ulTotalPacked;
	unsigned long m_ulTotalUnpacked;
	unsigned long m_ulTotalFiles;
	
	// does not apply to LHa
	//unsigned long m_ulMergeSize; 

	// archive-file crc ?
	unsigned int m_uiCrc;

	// descriptions (headers) of each entry in archive
	CLhHeader *m_pHeaders;
	
	void SeekHeader(CAnsiFile &ArchiveFile);
	
	tHuffBits GetDictionaryBits(const tCompressionMethod enMethod) const;
	
	void ExtractFile(CAnsiFile &ArchiveFile, LzHeader *pHeader, CAnsiFile &OutFile);

public slots:
	
	//void SetArchiveFile(QString szArchive);
	void SetConversionCodec(QTextCodec *pCodec);
	//void FileLocated(LzHeader *pHeader);

	bool Extract(QString &szExtractPath);
	bool List(QLhALib::tArchiveEntryList &lstArchiveInfo);
	bool Test();
	
	bool AddFiles(QStringList &lstFiles);
	
	/*
signals:
	void message(QString);
	void warning(QString);
	void error(QString);
	void fatal_error(QString);
	*/

public:
	// information about archive file itself
	QString GetArchiveFileName() const
	{
		return m_szArchive;
	}
	size_t GetArchiveFileSize() const
	{
		return m_nFileSize;
	}

	// statistical information access to caller
	unsigned long GetTotalSizeUnpacked() const
	{ 
		return m_ulTotalUnpacked; 
	}
	unsigned long GetTotalSizePacked() const
	{ 
		return m_ulTotalPacked; 
	}
	unsigned long GetTotalFileCount() const
	{ 
		return m_ulTotalFiles; 
	}
	
	/* does not apply to LHA
	unsigned long GetMergeSize() const
	{ 
		return m_ulMergeSize; 
	}
	*/
	
};

#endif // LHARCHIVE_H

