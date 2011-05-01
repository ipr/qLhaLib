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
#include <QList>
#include <QString>
#include <QStringList>

#include "AnsiFile.h"
#include "LhHeader.h"

/*
// for each file entry in archive
class CFileEntry
{
public:
	CFileEntry(void)
	{
	}
};
*/
//typedef QList<CFileEntry> tFileList;

// temp
//typedef QList<LzHeader*> tFileList;

// fwd. decl. for parent
class QLhALib;

class QTextCodec;


class CLhArchive : public QObject
{
	Q_OBJECT

public:
    CLhArchive(QLhALib *pParent, QString &szArchive);
	virtual ~CLhArchive(void);
	
protected:
	QString m_szCurrentArchive;
	size_t m_nArchiveFileSize;
	
	// some statistics of archive
	unsigned long m_ulPackedSizeTotal;
	unsigned long m_ulUnpackedSizeTotal;
	unsigned long m_ulFileCountTotal;
	
	CLhHeader m_Headers;
	//tFileList m_FileList;
	
	void SeekHeader(CAnsiFile &ArchiveFile);

public slots:
	
	//void SetArchiveFile(QString szArchive);
	void SetConversionCodec(QTextCodec *pCodec);

	bool Extract(QString &szExtractPath);
	bool List();
	bool Test();
	
	bool AddFiles(QStringList &lstFiles);
	
signals:
	void message(QString);
	void warning(QString);
	void error(QString);
	void fatal_error(QString);

	// file added to archive
	//void FileAdded(QString);
	// file extracted from archive
	//void FileExtracted(QString);
	
	// file found in archive
	//void FileLocated(CFileEntry);
	
};

#endif // LHARCHIVE_H

