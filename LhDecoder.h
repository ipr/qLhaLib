///////////////////////////////////////////
//
// CLhDecoder : extract&decode file(s) from LhA-archive file to disk
//
// Ilkka Prusi 2011
//


#ifndef LHDECODER_H
#define LHDECODER_H

#include <QObject>
#include <QString>
#include <QMap>

#include "AnsiFile.h"
#include "LhaTypeDefs.h"
#include "LhHeader.h"
#include "crcio.h"


// interface class for decoding methods,
// must be abstract
//
class CLhDecoder
{
protected:
	// actual implementations
	//decode_0_start();
	
	unsigned char *m_pDecodeTable;

	// for output, may grow
	CReadBuffer m_OutBuf;
	
	// size of decoded output (should be smaller than buffer)
	size_t m_nDecodedSize;
	
public:
	CLhDecoder(void)
		: m_pDecodeTable(nullptr)
		, m_OutBuf()
		, m_nDecodedSize(0)
	{}
	virtual ~CLhDecoder(void)
	{}

	void CreateDecoder() = 0; // only called once by container
	
	//void DecodeStart() = 0;
	//void DecodeStart(size_t nActualRead, CReadBuffer &InBuf) = 0;
	
	//void DecodeEnd() = 0;
	
	unsigned char *GetDecoded()
	{
		return m_OutBuf.GetBegin();
	}
	
	size_t GetDecodedSize() const
	{
		// TODO:
		return m_nDecodedSize;
	}
};

class CLhDecodeLh1 : public CLhDecoder
{
public:
	CLhDecodeLh1(void)
		: CLhDecoder()
	{}
	
	void CreateDecoder()
	{
		//m_pDecodeTable = new unsigned char[size];
	}
	
	/*
	void DecodeStart()
	{
		m_OutBuf.PrepareBuffer(input * x); // prepare for largest possible?
		CLhDecode::decode_0_start();
	}
	*/
	
};
class CLhDecodeLh2 : public CLhDecoder
{
public:
	CLhDecodeLh2(void)
		: CLhDecoder()
	{}
	
	void CreateDecoder()
	{
		//m_pDecodeTable = new unsigned char[size];
	}
};
class CLhDecodeLh3 : public CLhDecoder
{
public:
	CLhDecodeLh3(void)
		: CLhDecoder()
	{}
	
	void CreateDecoder()
	{
		//m_pDecodeTable = new unsigned char[size];
	}
};
class CLhDecodeLh4 : public CLhDecoder
{
public:
	CLhDecodeLh4(void)
		: CLhDecoder()
	{}
	
	void CreateDecoder()
	{
		//m_pDecodeTable = new unsigned char[size];
	}
};
class CLhDecodeLh5 : public CLhDecoder
{
public:
	CLhDecodeLh5(void)
		: CLhDecoder()
	{}
	
	void CreateDecoder()
	{
		//m_pDecodeTable = new unsigned char[size];
	}
};
class CLhDecodeLh6 : public CLhDecoder
{
public:
	CLhDecodeLh6(void)
		: CLhDecoder()
	{}
	
	void CreateDecoder()
	{
		//m_pDecodeTable = new unsigned char[size];
	}
};
class CLhDecodeLh7 : public CLhDecoder
{
public:
	CLhDecodeLh7(void)
		: CLhDecoder()
	{}
	
	void CreateDecoder()
	{
		//m_pDecodeTable = new unsigned char[size];
	}
};
class CLhDecodeLzs : public CLhDecoder
{
public:
	CLhDecodeLzs(void)
		: CLhDecoder()
	{}
	
	void CreateDecoder()
	{
		//m_pDecodeTable = new unsigned char[size];
	}
};
class CLhDecodeLz5 : public CLhDecoder
{
public:
	CLhDecodeLz5(void)
		: CLhDecoder()
	{}
	
	void CreateDecoder()
	{
		//m_pDecodeTable = new unsigned char[size];
	}
};


#endif // LHDECODER_H
