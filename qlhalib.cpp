#include "qlhalib.h"

#include <exception>

// entry point for actual handling code
#include "LhArchive.h"

#include <QTextCodec>


///////////// protected

void QLhALib::PrepareArchive(QString &szArchive)
{
	if (m_pLhaHandler != nullptr)
	{
		if (m_pLhaHandler->GetArchiveFileName() == szArchive)
		{
			// same archive already -> nothing to do?
			m_pLhaHandler->SetConversionCodec(m_pTextCodec);
			return;
		}
		
		delete m_pLhaHandler;
	}
	
	m_pLhaHandler = new CLhArchive(this, szArchive);
	m_pLhaHandler->SetConversionCodec(m_pTextCodec);
	
	//connect(m_pLhaHandler, SIGNAL(message(QString)), this, SIGNAL(message(QString)));
	//connect(m_pLhaHandler, SIGNAL(warning(QString)), this, SIGNAL(warning(QString)));
	//connect(m_pLhaHandler, SIGNAL(error(QString)), this, SIGNAL(error(QString)));
	//connect(m_pLhaHandler, SIGNAL(fatal_error(QString)), this, SIGNAL(fatal_error(QString)));
}


///////////// public

QLhALib::QLhALib(QObject *parent)
	: QObject(parent)
	, m_pLhaHandler(nullptr)
	, m_pTextCodec(nullptr)
{
}

QLhALib::~QLhALib()
{
	if (m_pLhaHandler != nullptr)
	{
		delete m_pLhaHandler;
	}
}

///////// various operation flags and options

void QLhALib::SetArchive(QString szArchive)
{
	PrepareArchive(szArchive);
}

// convert file names from given code page
// to unicode (helper for user of library)
//
void QLhALib::SetConversionCodec(QTextCodec *pCodec)
{
	m_pTextCodec = pCodec;
	if (m_pLhaHandler != nullptr)
	{
		m_pLhaHandler->SetConversionCodec(pCodec);
	}
}

///////// actual operations below

bool QLhALib::Extract(QString &szExtractPath)
{
	try
	{
		return m_pLhaHandler->Extract(szExtractPath);
	}
	catch (std::exception &exp)
	{
		emit fatal_error(exp.what());
	}
	return false;
}

bool QLhALib::List(QLhALib::tArchiveEntryList &lstArchiveInfo)
{
	try
	{
		return m_pLhaHandler->List(lstArchiveInfo);
	}
	catch (std::exception &exp)
	{
		emit fatal_error(exp.what());
	}
	return false;
}

bool QLhALib::Test()
{
	try
	{
		return m_pLhaHandler->Test();
	}
	catch (std::exception &exp)
	{
		emit fatal_error(exp.what());
	}
	return false;
}

bool QLhALib::AddFiles(QStringList &lstFiles)
{
	try
	{
		return m_pLhaHandler->AddFiles(lstFiles);
	}
	catch (std::exception &exp)
	{
		emit fatal_error(exp.what());
	}
	return false;
}

QString QLhALib::GetArchiveFileName()
{
	return m_pLhaHandler->GetArchiveFileName();
}

size_t QLhALib::GetArchiveFileSize()
{
	return m_pLhaHandler->GetArchiveFileSize();
}

unsigned long QLhALib::GetTotalSizeUnpacked() 
{ 
	return m_pLhaHandler->GetTotalSizeUnpacked(); 
}

unsigned long QLhALib::GetTotalSizePacked() 
{ 
	return m_pLhaHandler->GetTotalSizePacked(); 
}

unsigned long QLhALib::GetTotalFileCount() 
{ 
	return m_pLhaHandler->GetTotalFileCount(); 
}

