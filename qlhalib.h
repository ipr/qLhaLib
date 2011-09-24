/////////////////////////////////////////////
//
// QLhALib : exported interface to LhA library
// 
// Ilkka Prusi 2011
//
// Modified to be usable within other programs as a reusable library.
// Original code modified and compiled into stand-alone executable program (command-line).
//
// Modifications include (not limited to):
//
// 1) function definition updates
// for example, following does not always compile:
//   char *strdup(buf)
//      const char *buf;
//   { ..function code.. }
//
// 2) removed unnecessary ifdef's
// think we can expect to have working memcpy(), memset() and memmove()
// on just about any platform now..
// (usually compiler does better optimization of it anyway.)
// also must expect some features for C++ support
// and must be able to build Qt..
//
// 3) keyword-changes
// keywords that are not allowed,
// for example, 'interface' is reserved by MS VC++
//
// 4) better scoping
// trying to figure out all 
// those external/global variables all over the place..
//
// 5) reduction of static variables/methods
// hard to follow AND dll-code really does not like statics
// (consider case where multiple threads/processes uses same library..)
//
// 6) buffering/IO changes
//
// 7) standard-library support
// C-style buffers replaces (in places)
// by std::string, std::list and std::vector, for example.
//
// 8) removed console-output
// does not belong in a library-code, which is what this should become..
//
// 9) code-style changed as side-effect..
// changed style while figuring out what code does..
//
//
// Based on LhA source code: lha-1.14i-ac20040929
//
// Copyrights:
// LHarc    for UNIX  V 1.02  Copyright(C) 1989  Y.Tagawa
// LHx      for MSDOS V C2.01 Copyright(C) 1990  H.Yoshizaki
// LHx(arc) for OSK   V 2.01  Modified     1990  Momozou
// LHa      for UNIX  V 1.00  Copyright(C) 1992  Masaru Oki
// LHa      for UNIX  V 1.14  Modified     1995  Nobutaka Watazaki
// LHa      for UNIX  V 1.14i Modified     2000  Tsugio Okamoto
//                    Autoconfiscated 2001-2004  Koji Arai
//
// usage: lha [-]<commands>[<options>] [-<options> ...] archive_file [file...]
//   commands:  [axelvudmcpt]
//   options:   [q[012]vnfto[567]dizg012e[w=<dir>|x=<pattern>]]
// commands:                           options:
//  a   Add(or replace) to archive      q{num} quiet (num:quiet mode)
//  x,e EXtract from archive            v  verbose
//  l,v List / Verbose List             n  not execute
//  u   Update newer files to archive   f  force (over write at extract)
//  d   Delete from archive             t  FILES are TEXT file
//  m   Move to archive (means 'ad')    o[567] compression method (a/u/c)
//  c   re-Construct new archive        d  delete FILES after (a/u/c)
//  p   Print to STDOUT from archive    i  ignore directory path (x/e)
//  t   Test file CRC in archive        z  files not compress (a/u/c)
//                                      g  Generic format (for compatibility)
//                                         or not convert case when extracting
//                                      0/1/2 header level (a/u/c)
//                                      e  TEXT code convert from/to EUC
//                                      w=<dir> specify extract directory (x/e)
//                                      x=<pattern>  eXclude files (a/u/c)
// 
// 


#ifndef QLHALIB_H
#define QLHALIB_H

#include "qLhA_global.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QDateTime>

// fwd. decl. class for entry point
class CLhArchive;
class QTextCodec;

class QLHASHARED_EXPORT QLhALib : public QObject
{
	Q_OBJECT

private:
	CLhArchive *m_pLhaHandler;
	QTextCodec *m_pTextCodec;
	
protected:
	
	void PrepareArchive(QString &szArchive);
	
public:
    QLhALib(QObject *parent = 0);
    virtual ~QLhALib();

	// for each file entry in archive (for caller)
	class CArchiveEntry
	{
	public:
		CArchiveEntry(void)
			: m_uiCrc(0)
			, m_ulUnpackedSize(0)
			, m_ulPackedSize(0)
		    , m_ucHeaderLevel(0)
			, m_Stamp()
			, m_szFileName()
			//, m_szPathName()
			, m_szPackMode()
			, m_extendType()
			, m_szComment()
			, m_szUser()
			, m_szGroup()
			, m_unix_uid(0)
			, m_unix_gid(0)
		{
		}
	
		// CRC from archive,
		// only 16-bit CRC in LHa
		unsigned int m_uiCrc;
		
		// unpacked size of file
		unsigned long m_ulUnpackedSize;
	
		// compressed size offile
		unsigned long m_ulPackedSize;
		
		// header "level" (type)
		unsigned char m_ucHeaderLevel;
	
		// "last modified" time usually
		QDateTime m_Stamp;
		
		// name of entry from archive
		QString m_szFileName;
		
		//QString m_szPathName;
		//QString m_szRealName; // if symlink?

		// -l??- string of packing mode, e.g. '-lh5-', '-lz0-'
		QString m_szPackMode;

		// temp, string for now..
		// for diagnostics, show extended-header type (if errors occur)
		QString m_extendType;
		
		// file-related comment from archive (if any)
		// (usually in Amiga-packed files)
		QString m_szComment;
		
		// Unix-style user&group of file
		QString m_szUser;
		QString m_szGroup;
		
		unsigned short m_unix_uid;
		unsigned short m_unix_gid;
		
		// generic way to give protection flags to caller?
		// (whichever os, gui or console..)
		
	};
	
	// simplest way to list all files in archive for caller
	typedef QList<CArchiveEntry> tArchiveEntryList;
	
	
public slots:
	// various operation flags and options accessible below

	void SetArchive(QString szArchive);

	// helper for user of library:
	// convert filenames from given codepage to unicode
	void SetConversionCodec(QTextCodec *pCodec);

	//////////////////	
	// actual operations below
	bool Extract(QString &szExtractPath);
	bool ExtractSelected(QString &szExtractPath, QStringList &lstFiles);
	bool ExtractToBuffer(QString &szFileEntry, QByteArray &outArray);
	bool List(QLhALib::tArchiveEntryList &lstArchiveInfo);
	bool Test();

signals:
	void message(QString);
	void warning(QString);
	
	// info to caller on exception
	void fatal_error(QString);

public:
	// information about archive file itself
	QString GetArchiveFileName();
	size_t GetArchiveFileSize();

	// statistical information access to caller
	unsigned long GetTotalSizeUnpacked();
	unsigned long GetTotalSizePacked();
	unsigned long GetTotalFileCount();
	
};

#endif // QLHALIB_H
