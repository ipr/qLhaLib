#include "qlhalib.h"

// entry point for actual handling code
#include "LhArchive.h"

#include <QTextCodec>


///////////// protected

void QLhALib::PrepareArchive(QString &szArchive)
{
	if (m_pLhaHandler != nullptr)
	{
		delete m_pLhaHandler;
	}
	
	m_pLhaHandler = new CLhArchive(this, szArchive);
	m_pLhaHandler->SetConversionCodec(m_pTextCodec);
	
	connect(m_pLhaHandler, SIGNAL(message(QString)), this, SIGNAL(message(QString)));
	connect(m_pLhaHandler, SIGNAL(warning(QString)), this, SIGNAL(warning(QString)));
	connect(m_pLhaHandler, SIGNAL(error(QString)), this, SIGNAL(error(QString)));
	connect(m_pLhaHandler, SIGNAL(fatal_error(QString)), this, SIGNAL(fatal_error(QString)));
	
	//connect(m_pLhaHandler, SIGNAL(FileLocated(CFileEntry)), this, SIGNAL(FileLocated(CFileEntry)));
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
	return m_pLhaHandler->Extract(szExtractPath);
}

bool QLhALib::List()
{
	return m_pLhaHandler->List();
}

bool QLhALib::Test()
{
	return m_pLhaHandler->Test();
}

bool QLhALib::AddFiles(QStringList &lstFiles)
{
	return m_pLhaHandler->AddFiles(lstFiles);
}


