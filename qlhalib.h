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
// 3) standard-library support
// C-style buffers replaces (in places)
// by std::string, std::list and std::vector, for example.
//
// 4) removed console-output
// does not belong in a library-code, which is what this should become..
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
	
	// operation flags and options
	// TODO: see what should be supported..
	
	//bool m_bVerbose; // v
	//bool m_bNotExecute; // n 
	//bool m_bForceOverwrite; // f
	//bool m_bTextMode; // t
		
	//bool m_bDeleteAfter; // d
	//bool m_bNoFileCompress; // z
	//bool m_bGenericFormat; // g
	//bool m_bEucTextConversion; // e

	//int m_iQuietMode; // q
	//tCompressionMethod m_enCompressionMethod;
	//tHeaderLevel m_enHeaderLevel;
	
	// archive-file (always necessary)
	// ->moved
	//QString m_szArchive;
	
	// extraction path (when necessary)
	//QString m_szExtractPath; // w

	// exclude files by pattern
	//QString m_szExcludePattern; // x

	//QString m_szIgnorePath; // i
	
	// work/temp folder path to use (if necessary)
	//QString m_szWorkPath;
	
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
			, m_bPackedSizeAvailable(true)
			, m_Stamp()
			, m_szFileName()
			//, m_szPathName()
			, m_szPackMode()
			//, m_szComment()
		{
		}
	
		// CRC from archive,
		// only 16-bit CRC in LHa
		// (32-bit CRC in LZX)
		unsigned int m_uiCrc;
		
		// unpacked size of file
		unsigned long m_ulUnpackedSize;
	
		// compressed size offile
		unsigned long m_ulPackedSize;
	
		// packed-size is not available for merged files (LZX)
		// -> not supported by LHa..
		bool m_bPackedSizeAvailable;
		
		// "last modified" time usually
		QDateTime m_Stamp;
		
		// name of entry from archive
		QString m_szFileName;
		
		//QString m_szPathName;
		//QString m_szRealName; // if symlink?

		// -l??- string, e.g. '-lh5-', '-lz0-'
		QString m_szPackMode;
		
		// uncompressed file-comment from archive (if any)
		//QString m_szComment;
	};
	
	// simplest way to list all files in archive for caller
	typedef QList<CArchiveEntry> tArchiveEntryList;
	
	
public slots:
	// various operation flags and options accessible below
	// TODO: see what should be supported..
	
	//void SetQuiet(int iMode) {};
	//void SetCompression(int iMode) {};
	//void SetHeaderLevel(int iLevel) {};
	//void SetVerbose(bool bStatus);
	//void SetNoExecute(bool bStatus);
	//void SetForceOverwrite(bool bStatus);
	//void SetTextMode(bool bStatus);
	//void SetDeleteAfter(bool bStatus);
	//void SetNoCompress(bool bStatus);
	//void SetGenericFormat(bool bStatus);
	//void SetEucTextConversion(bool bStatus);

	void SetArchive(QString szArchive);
	//void SetExtractPath(QString szPath);
	
	//void SetExcludePattern(QString szPattern);
	//void SetIgnorePath(QString szPath);
	
	//void SetWorkPath(QString szPath);

	// helper for user of library:
	// convert filenames from given codepage to unicode
	void SetConversionCodec(QTextCodec *pCodec);

	//////////////////	
	// actual operations below
	bool Extract(QString &szExtractPath);
	bool List(QLhALib::tArchiveEntryList &lstArchiveInfo);
	bool Test();

	bool AddFiles(QStringList &lstFiles);
	//bool Update();
	
signals:
	void message(QString);
	void warning(QString);
	void error(QString);
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
