///////////////////////////////////////////
//
// CLhDecoder : extract&decode file(s) from LhA-archive file to disk
//
// Ilkka Prusi 2011
//


#ifndef LHDECODER_H
#define LHDECODER_H

#include <stdint.h>
#include <limits.h>

#include <QObject>
#include <QString>
#include <QMap>

#include "AnsiFile.h"
#include "LhaTypeDefs.h"
#include "LhHeader.h"
#include "crcio.h"

#define USHRT_BIT           16  /* (CHAR_BIT * sizeof(ushort)) */

class BitIo
{
public:
	unsigned short bitbuf;

	unsigned char subbitbuf;
	unsigned char bitcount;

	/* slide.c */
	int unpackable; // encoding only?
	size_t origsize;
	size_t compsize;

	
	// TODO:?
	//CReadBuffer *m_pReadBuf;
	//CReadBuffer *m_pWriteBuf;
	
	// 
	unsigned char *m_pReadBuffer;
	unsigned char *m_pWriteBuffer;

	size_t m_nReadPos;
	size_t m_nWritePos;
	

	unsigned short peekbits(unsigned char n)
	{
		return (bitbuf >> (sizeof(bitbuf)*8 - (n)));
	}

	void fillbuf(unsigned char n);
	
	unsigned short getbits(unsigned char n)
	{
		unsigned short x;
		x = bitbuf >> (2 * CHAR_BIT - n);
		fillbuf(n);
		return x;
	}
	
	void putcode(unsigned char n, unsigned short x);
	
	/* Write rightmost n bits of x */
	void putbits(unsigned char n, unsigned short x)
	{
		unsigned short y = x;
		y <<= USHRT_BIT - n;
		putcode(n, y);
	}
	
	void init_getbits()
	{
		bitbuf = 0;
		subbitbuf = 0;
		bitcount = 0;
		fillbuf(2 * CHAR_BIT);
	}

	void init_putbits()
	{
		bitcount = CHAR_BIT;
		subbitbuf = 0;
	}
	
    BitIo()
		: m_pReadBuffer(nullptr)
		, m_pWriteBuffer(nullptr)
		, m_nReadPos(0)
		, m_nWritePos(0)
		, bitbuf(0)
		, subbitbuf(0)
	{}
	
};


// interface class for decoding methods,
// must be abstract
//
class CLhDecoder
{
protected:
	// actual implementations
	//decode_0_start();

	BitIo m_BitIo;
	
	unsigned char *m_pDecodeTable;

	// for output, may grow
	CReadBuffer m_OutBuf;
	
	// size of decoded output (should be smaller than buffer)
	size_t m_nDecodedSize;

	// amount of packed data read
	size_t m_nReadPackedSize;
	
public:
	CLhDecoder(void)
		: m_BitIo()
		, m_pDecodeTable(nullptr)
		, m_OutBuf()
		, m_nDecodedSize(0)
		, m_nReadPackedSize(0)
	{}
	virtual ~CLhDecoder(void)
	{}

	virtual void CreateDecoder() = 0; // only called once by container
	
	//virtual void DecodeStart() = 0;
	//virtual void DecodeStart(size_t nActualRead, CReadBuffer &InBuf) = 0;
	
	//virtual void DecodeEnd() = 0;
	
	unsigned char *GetDecoded()
	{
		return m_OutBuf.GetBegin();
	}

	size_t GetDecodedSize() const
	{
		// TODO:
		return m_nDecodedSize;
	}
	
	size_t GetReadPackedSize() const
	{
		// TODO:
		return m_nReadPackedSize;
	}
};

class CLhDecodeLh1 : public CLhDecoder
{
public:
	CLhDecodeLh1(void)
		: CLhDecoder()
	{}
	virtual ~CLhDecodeLh1(void)
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
	virtual ~CLhDecodeLh2(void)
	{}
	
	virtual void CreateDecoder()
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
	virtual ~CLhDecodeLh3(void)
	{}
	
	virtual void CreateDecoder()
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
	virtual ~CLhDecodeLh4(void)
	{}
	
	virtual void CreateDecoder()
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
	virtual ~CLhDecodeLh5(void)
	{}
	
	virtual void CreateDecoder()
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
	virtual ~CLhDecodeLh6(void)
	{}
	
	virtual void CreateDecoder()
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
	virtual ~CLhDecodeLh7(void)
	{}
	
	virtual void CreateDecoder()
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
	virtual ~CLhDecodeLzs(void)
	{}
	
	virtual void CreateDecoder()
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
	virtual ~CLhDecodeLz5(void)
	{}
	
	virtual void CreateDecoder()
	{
		//m_pDecodeTable = new unsigned char[size];
	}
};


#endif // LHDECODER_H
