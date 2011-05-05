//////////////////////////////////////
//
// AnsiFile.h
//
// Ilkka Prusi
// ilkka.prusi@gmail.com
//

#ifndef _ANSIFILE_H_
#define _ANSIFILE_H_

#include <string>
#include <exception>


// exception-classes for error cases
class IOException : public std::exception
{
public:
	IOException(const char *szMessage)
		: std::exception(szMessage)
	{
	}
};

class ArcException : public std::exception
{
protected:
	std::string m_szData;
public:
	ArcException(const char *szMessage, const std::string &szData)
		: std::exception(szMessage)
		, m_szData(szData)
	{
	}
	ArcException(const char *szMessage, const size_t nData)
		: std::exception(szMessage)
		, m_szData()
	{
		// TODO:
		//m_szData = std::lexical_cast<std::string>(nData);
		//m_szData = ltoa(nData;
	}
	std::string GetData()
	{
		return m_szData;
	}
};

class CReadBuffer
{
private:
	enum tSizes
	{
		INITIAL_READ_BUFFER_SIZE = 16384,
		MAX_READ_BUFFER_SIZE = 1024*1024
	};

	unsigned char *m_pReadBuffer;
	size_t m_nReadBufferSize;
	
	//size_t m_nMaxBufferSize; // limit of growing buffer..
	
	//bool m_bKeepExisting; // keep existing data if growing buffer..
	//bool m_bPageAlign; // page-size aligned allocations

	inline void CreateBuffer(const size_t nMinSize)
	{
		// allocate new
		m_pReadBuffer = new unsigned char[nMinSize];
		m_nReadBufferSize = nMinSize;
		::memset(m_pReadBuffer, 0, m_nReadBufferSize);
	}

public:
	CReadBuffer(void) 
		: m_pReadBuffer(nullptr)
		, m_nReadBufferSize(0)
		//, m_nMaxBufferSize(MAX_READ_BUFFER_SIZE)
	{
		CreateBuffer(INITIAL_READ_BUFFER_SIZE);
	}
	
	CReadBuffer(const size_t nMinsize) 
		: m_pReadBuffer(nullptr)
		, m_nReadBufferSize(0)
		//, m_nMaxBufferSize(MAX_READ_BUFFER_SIZE)
	{
		CreateBuffer(nMinsize);
	}
	
	/*
	CReadBuffer(const size_t nMinsize, 
				const size_t nMaxsize) 
		: m_pReadBuffer(nullptr)
		, m_nReadBufferSize(0)
		//, m_nMaxBufferSize(nMaxsize)
	{
		PrepareBuffer(nMinsize);
	}
	*/
	
	~CReadBuffer(void) 
	{
		if (m_pReadBuffer != nullptr)
		{
			delete m_pReadBuffer;
		}
	}

	// allocate or grow if necessary
	void PrepareBuffer(const size_t nMinSize)
	{
		if (m_pReadBuffer == nullptr
			|| m_nReadBufferSize == 0)
		{
			CreateBuffer(nMinSize);
			return;
		}

		// growing, do we need this..?
		if (m_pReadBuffer != nullptr
			&& m_nReadBufferSize < nMinSize)
		{
			size_t nNewSize = nMinSize;
			/*
			if (nNewSize > m_nMaxBufferSize)
			{
				nNewSize = m_nMaxBufferSize;
			}
			*/
			delete m_pReadBuffer; // destroy old smaller
			
			m_pReadBuffer = new unsigned char[nNewSize];
			m_nReadBufferSize = nNewSize;
			//return;
		}

		// otherwise just clear existing (keep existing)
		::memset(m_pReadBuffer, 0, m_nReadBufferSize);
	}


	// note: don't make it const:
	// allow modify to read into it..
	//
	unsigned char *GetBegin()
	{
		return m_pReadBuffer;
	}
	
	// reduce repeated code -> count end ptr
	unsigned char *GetEnd()
	{
		return m_pReadBuffer + m_nReadBufferSize;
	}

	// reduce repeated code -> count to given offset from start
	unsigned char *GetAt(const size_t nOffset)
	{
		return m_pReadBuffer + nOffset;
	}
	
	// current allocation
	//
	size_t GetSize() const
	{
		return m_nReadBufferSize;
	}
};


// ANSI-C style file-API helper
class CAnsiFile
{
protected:
	FILE *m_pFile;
	size_t m_nFileSize;
	bool m_bIsWritable;

	size_t GetSizeOfFile()
	{
		if (m_pFile == NULL)
		{
			return 0;
		}

		// should be at start (0)
		long lCurrent = ftell(m_pFile);

		// locate end
		if (fseek(m_pFile, 0L, SEEK_END) != 0)
		{
			return 0;
		}
		size_t nSize = ftell(m_pFile);

		// return to start
		fseek(m_pFile, lCurrent, SEEK_SET);
		return nSize;
	}

public:
	CAnsiFile(void)
		: m_pFile(NULL)
		, m_nFileSize(0)
		, m_bIsWritable(false)
	{}
	CAnsiFile(const std::string &szFile, bool bWritable = false)
		: m_pFile(NULL)
		, m_nFileSize(0)
		, m_bIsWritable(bWritable)
	{
		Open(szFile, bWritable);
	}
	~CAnsiFile(void)
	{
		Close();
	}

	operator FILE *() const
	{
		return m_pFile;
	}

	bool Open(const std::string &szFile, bool bWritable = false)
	{
		Close(); // close previous (if any)

		if (bWritable == false)
		{
			m_pFile = fopen(szFile.c_str(), "rb");
			m_nFileSize = GetSizeOfFile();
		}
		else
		{
			// size zero if new file..
			m_pFile = fopen(szFile.c_str(), "wb");
		}
		m_bIsWritable = bWritable;
		return IsOk();
	}

	void Close()
	{
		if (m_pFile != NULL)
		{
			fclose(m_pFile);
			m_pFile = NULL;
		}
	}

	size_t GetSize()
	{
		// get size as it was in start
		if (m_bIsWritable == false)
		{
			return m_nFileSize;
		}
		// determine current size
		return GetSizeOfFile();
	}

	bool IsOk()
	{
		if (m_pFile == NULL)
		{
			return false;
		}

		if (ferror(m_pFile) != 0) 
		{
			return false;
		}
		return true;
	}

	bool Flush()
	{
		if (fflush(m_pFile) != 0)
		{
			return false;
		}
		return true;
	}

	bool Write(const void *pBuffer, const size_t nBytes)
	{
		size_t nWritten = fwrite(pBuffer, 1, nBytes, m_pFile);
		if (IsOk() == false
			|| nWritten < nBytes)
		{
			return false;
		}
		return Flush();
	}

	bool Read(void *pBuffer, const size_t nBytes)
	{
		size_t nRead = fread(pBuffer, 1, nBytes, m_pFile);
		if (IsOk() == false
			|| nRead < nBytes)
		{
			return false;
		}
		return true;
	}
	
	bool Read(CReadBuffer &Buffer)
	{
		return Read(Buffer.GetBegin(), Buffer.GetSize());
	}
	
	bool Tell(long &lPos)
	{
		lPos = ftell(m_pFile);
		if (lPos < 0)
		{
			return false;
		}
		return true;
	}

	bool Seek(const size_t nBytes, const int iOrigin)
	{
		if (fseek(m_pFile, nBytes, iOrigin) != 0)
		{
			return false;
		}
		return true;
	}
	
	/*
	bool SeekOff(const long lOffset, const int iOrigin)
	{
		if (fseeko(m_pFile, lOffset, iOrigin) != 0)
		{
			return false;
		}
		return true;
	}
	*/
};

class CPathHelper
{
public:
	static bool MakePath(const std::string &szPath);
	static bool MakePathToFile(const std::string &szOutFile);
};

#endif // ifndef _ANSIFILE_H_

