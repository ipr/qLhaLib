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
#include "LzHeader.h"


// for each file entry in archive
class CFileEntry
{
public:
	CFileEntry(void)
	{
	}
};

typedef QList<CFileEntry> tFileList;

// fwd. decl. for parent
class QLhALib;

class CLhArchive : public QObject
{
	Q_OBJECT

public:
    CLhArchive(QLhALib *pParent);
	
protected:
	QString m_szCurrentArchive;
	size_t m_nArchiveFileSize;
	
	// some statistics of archive
	unsigned long m_ulPackedSizeTotal;
	unsigned long m_ulUnpackedSizeTotal;
	unsigned long m_ulFileCountTotal;
	
	CLzHeader m_FileHeader;
	tFileList m_FileList;
	
	void SeekHeader(CAnsiFile &ArchiveFile);
	
signals:
	void message(QString);
	void warning(QString);
	void error(QString);
	void fatal_error(QString);

public slots:
	void SetArchiveFile(QString szArchive);

public:
	bool Extract(QString &szExtractPath);
	bool List();
	bool Test();
	
	bool AddFiles(QStringList &lstFiles);
	
};

#endif // LHARCHIVE_H

