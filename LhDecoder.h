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
#include "Huffman.h"



//////// decoder base

// interface/base class for decoding methods,
// must be abstract
//
class CLhDecoder
{
private:
	// actual implementations

	//// decode start
	// -lh1- and -lh2- have specific cases
	// -lh3-, static huffman (0)
	// -lh4- .. -lh7-, static huffman (1)
	//// decode C
	// -lh1- and -lh2-, dynamic huffman
	// -lh3-, static huffman (0)
	// -lh4- .. -lh7-, static huffman (1)
	//// decode P
	// -lh2-, dynamic huffman
	// -lh1- and -lh3-, static huffman (0)
	// -lh4- .. -lh7-, static huffman (1)

	
protected:
	
	CCrcIo m_crcio;
	unsigned int m_uiCrc;
	
	// dictionary-related
	unsigned long m_dicsiz;
	unsigned char *m_dtext;
	unsigned int m_dicsiz_1;
	unsigned int m_adjust;

	// was global.. only actually used with -lzs- and -lz5-
    unsigned long m_loc;
	
public:
	CLhDecoder(void)
		: m_crcio()
		, m_uiCrc(0)
		, m_dicsiz(0)
		, m_dtext(nullptr)
		, m_dicsiz_1(0)
		, m_adjust(0)
		, m_loc(0)
	{}
	virtual ~CLhDecoder(void)
	{}

	// Create(): only called once by container (optional)
	virtual void CreateDecoder() {};

	// called before reusing
	virtual void InitClear()
	{
		m_uiCrc = 0;
		m_dicsiz = 0;
		m_dtext = nullptr;
		m_dicsiz_1 = 0;
		m_adjust = 0;
		m_loc = 0;
	}
	virtual void SetDict(unsigned long dicsiz, unsigned char *dtext, unsigned int dicsiz_1, unsigned int adjust)
	{
		m_dicsiz = dicsiz;
		m_dtext = dtext;
		m_dicsiz_1 = dicsiz_1;
		m_adjust = adjust;
	}
	
	// called on start of decode to set buffers
	virtual void SetBuffers(CReadBuffer *pReadBuf, CReadBuffer *pWriteBuf) = 0;

	// may be used during decoding
	virtual CReadBuffer *GetReadBuf() = 0;
	virtual CReadBuffer *GetWriteBuf() = 0;
	
	virtual void DecodeStart() = 0;
	virtual unsigned short DecodeC() = 0;
	virtual unsigned short DecodeP() = 0;
	

	// was global..
	// check if this is needed at all,
	// for now, keep behind interface to track usage
	unsigned long GetLoc() const
	{
		return m_loc;
	}
	void SetLoc(const unsigned long ulLoc)
	{
		m_loc = ulLoc;
	}
	
	unsigned int GetCrc() const
	{
		return m_uiCrc;
	}
	
	// 
	virtual void Decode(size_t &decode_count);
};

//////// decoders

// (note: -lh0-, -lhd- and -lz4- are "store only", no compression)

// -lh1-
// (dynamic huffman)
class CLhDecodeLh1 : public CLhDecoder, protected CShuffleHuffman
{
public:
	CLhDecodeLh1(void)
		: CLhDecoder()
		, CShuffleHuffman()
	{}
	virtual ~CLhDecodeLh1(void)
	{}

	virtual void CreateDecoder() {};
	
	virtual void SetBuffers(CReadBuffer *pReadBuf, CReadBuffer *pWriteBuf)
	{
		// set to where this was inherited from
		m_BitIo.m_pReadBuf = pReadBuf;
		m_BitIo.m_pWriteBuf = pWriteBuf;
	}
	virtual CReadBuffer *GetReadBuf()
	{
		return m_BitIo.m_pReadBuf;
	}
	virtual CReadBuffer *GetWriteBuf()
	{
		return m_BitIo.m_pWriteBuf;
	}
	
	virtual void DecodeStart();
	virtual unsigned short DecodeC();
	virtual unsigned short DecodeP();

};

// -lh2-
// (dynamic huffman)
class CLhDecodeLh2 : public CLhDecoder, protected CDynamicHuffman
{
public:
	CLhDecodeLh2(void)
		: CLhDecoder()
		, CDynamicHuffman()
	{}
	virtual ~CLhDecodeLh2(void)
	{}

	virtual void CreateDecoder() {};
	
	virtual void SetBuffers(CReadBuffer *pReadBuf, CReadBuffer *pWriteBuf)
	{
		// set to where this was inherited from
		m_BitIo.m_pReadBuf = pReadBuf;
		m_BitIo.m_pWriteBuf = pWriteBuf;
	}
	virtual CReadBuffer *GetReadBuf()
	{
		return m_BitIo.m_pReadBuf;
	}
	virtual CReadBuffer *GetWriteBuf()
	{
		return m_BitIo.m_pWriteBuf;
	}
	
	virtual void DecodeStart();
	virtual unsigned short DecodeC();
	virtual unsigned short DecodeP();
	
};

// -lh3-
// (static huffman routine 0 -> shuffle)
class CLhDecodeLh3 : public CLhDecoder, protected CShuffleHuffman
{
public:
	CLhDecodeLh3(void)
		: CLhDecoder()
		, CShuffleHuffman()
	{}
	virtual ~CLhDecodeLh3(void)
	{}

	virtual void CreateDecoder() {};
	
	virtual void SetBuffers(CReadBuffer *pReadBuf, CReadBuffer *pWriteBuf)
	{
		// set to where this was inherited from
		m_BitIo.m_pReadBuf = pReadBuf;
		m_BitIo.m_pWriteBuf = pWriteBuf;
	}
	virtual CReadBuffer *GetReadBuf()
	{
		return m_BitIo.m_pReadBuf;
	}
	virtual CReadBuffer *GetWriteBuf()
	{
		return m_BitIo.m_pWriteBuf;
	}
	
	virtual void DecodeStart();
	virtual unsigned short DecodeC();
	virtual unsigned short DecodeP();
};

/*
// -lh4- .. -lh7- (check virtual methods)
class CLhDecodeLh4 : public CLhDecoder
// -lh4- .. -lh7- (check virtual methods)
class CLhDecodeLh5 : public CLhDecoder
// -lh4- .. -lh7- (check virtual methods)
class CLhDecodeLh6 : public CLhDecoder
*/

// -lh4- .. -lh7- (same for each)
// (static huffman routine 1)
class CLhDecodeLh7 : public CLhDecoder, protected CStaticHuffman
{
public:
	CLhDecodeLh7(void)
		: CLhDecoder()
		, CStaticHuffman()
	{}
	virtual ~CLhDecodeLh7(void)
	{}

	virtual void CreateDecoder() {};
	
	virtual void SetBuffers(CReadBuffer *pReadBuf, CReadBuffer *pWriteBuf)
	{
		// set to where this was inherited from
		m_BitIo.m_pReadBuf = pReadBuf;
		m_BitIo.m_pWriteBuf = pWriteBuf;
	}
	virtual CReadBuffer *GetReadBuf()
	{
		return m_BitIo.m_pReadBuf;
	}
	virtual CReadBuffer *GetWriteBuf()
	{
		return m_BitIo.m_pWriteBuf;
	}
	
	virtual void DecodeStart();
	virtual unsigned short DecodeC();
	virtual unsigned short DecodeP();
};

// -lzs-
class CLhDecodeLzs : public CLhDecoder
{
protected:
	int flag, flagcnt;
	int m_matchpos;
	
	BitIo m_BitIo;
	
public:
	CLhDecodeLzs(void)
		: CLhDecoder()
		, flag(0)
		, flagcnt(0)
		, m_matchpos(0)
		, m_BitIo()
	{}
	virtual ~CLhDecodeLzs(void)
	{}
	
	virtual void CreateDecoder() {};
	
	virtual void SetBuffers(CReadBuffer *pReadBuf, CReadBuffer *pWriteBuf)
	{
		// set to where this was inherited from
		m_BitIo.m_pReadBuf = pReadBuf;
		m_BitIo.m_pWriteBuf = pWriteBuf;
	}
	virtual CReadBuffer *GetReadBuf()
	{
		return m_BitIo.m_pReadBuf;
	}
	virtual CReadBuffer *GetWriteBuf()
	{
		return m_BitIo.m_pWriteBuf;
	}
	
	virtual void DecodeStart();
	virtual unsigned short DecodeC();
	virtual unsigned short DecodeP();
};

// -lz5-
class CLhDecodeLz5 : public CLhDecoder
{
protected:
	int flag, flagcnt;
	int m_matchpos;

	BitIo m_BitIo;
	
public:
	CLhDecodeLz5(void)
		: CLhDecoder()
		, flag(0)
		, flagcnt(0)
		, m_matchpos(0)
		, m_BitIo()
	{}
	virtual ~CLhDecodeLz5(void)
	{}
	
	virtual void CreateDecoder() {};
	
	virtual void SetBuffers(CReadBuffer *pReadBuf, CReadBuffer *pWriteBuf)
	{
		// set to where this was inherited from
		m_BitIo.m_pReadBuf = pReadBuf;
		m_BitIo.m_pWriteBuf = pWriteBuf;
	}
	virtual CReadBuffer *GetReadBuf()
	{
		return m_BitIo.m_pReadBuf;
	}
	virtual CReadBuffer *GetWriteBuf()
	{
		return m_BitIo.m_pWriteBuf;
	}
	
	virtual void DecodeStart();
	virtual unsigned short DecodeC();
	virtual unsigned short DecodeP();
};


#endif // LHDECODER_H
