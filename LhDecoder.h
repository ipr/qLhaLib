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

	// buffer for dictionary text
	CReadBuffer m_DictionaryText;
	
	// dictionary-related
	unsigned long m_dicsiz;
	unsigned char *m_dtext;
	unsigned int m_dicsiz_1;
	unsigned int m_adjust;
	tHuffBits m_enBit;

	// was global.. only actually used with -lzs- and -lz5-
    unsigned long m_loc;
	
public:
	CLhDecoder(void)
		: m_crcio()
		, m_uiCrc(0)
		, m_DictionaryText(1024)
		, m_dicsiz(0)
		, m_dtext(nullptr)
		, m_dicsiz_1(0)
		, m_adjust(0)
		, m_loc(0)
		, m_enBit(LARC5_DICBIT) // default for backwards compatibility
	{}
	virtual ~CLhDecoder(void)
	{}

	// called before reusing
	void InitClear()
	{
		m_uiCrc = 0;
		m_dicsiz = 0;
		m_dtext = nullptr;
		m_dicsiz_1 = 0;
		m_adjust = 0;
		m_loc = 0;
		m_enBit = LARC5_DICBIT;
	}
	
	void InitDictionary(const tCompressionMethod enCompression, const tHuffBits enHuffBit)
	{
		m_enBit = enHuffBit;
		
		m_dicsiz = (1L << (int)m_enBit); // 
		
		// verify allocation sufficient, zeroing entire buffer
		m_DictionaryText.PrepareBuffer(m_dicsiz, false); // can remain larger, doesn't matter
		
		m_dtext = m_DictionaryText.GetBegin();

		// clear dictionary
		//memset(m_dtext, 0, m_dicsiz); // for broken archive only? why?
		memset(m_dtext, ' ', m_dicsiz);
		
		m_dicsiz_1 = m_dicsiz - 1; // why is this separate?
		
		m_adjust = 256 - THRESHOLD;
		if (enCompression == LARC_METHOD_NUM)
		{
			m_adjust = 256 - 2;
		}
	}
	
	// need to implement in inherited (bitio by inheritance..)
	virtual void InitBitIo(const size_t nPackedSize, const size_t nOriginalSize, CReadBuffer *pReadBuf, CReadBuffer *pWriteBuf)
	{
		// call implementation in inherited class
		BitIo *pIo = GetBitIo();
		
		// set to inherited bitio-handler
		pIo->compsize = nPackedSize;
		pIo->origsize = nOriginalSize;
		pIo->m_pReadBuf = pReadBuf;
		pIo->m_pWriteBuf = pWriteBuf;
	
		// use this here, remove elsewhere..
		//pIo->init_getbits();
	}
	
	// may be used during decoding
	virtual CReadBuffer *GetReadBuf() = 0;
	virtual CReadBuffer *GetWriteBuf() = 0;
	virtual BitIo *GetBitIo() = 0;
	
	virtual void DecodeStart() = 0;
	virtual unsigned short DecodeC(size_t &decode_count) = 0;
	virtual unsigned short DecodeP(size_t &decode_count) = 0;

	virtual void DecodeFinish()
	{
		if (m_loc != 0) 
		{
			m_uiCrc = m_crcio.calccrc(m_uiCrc, m_dtext, m_loc);
			GetWriteBuf()->Append(m_dtext, m_loc);
		}
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

	virtual CReadBuffer *GetReadBuf()
	{
		return m_BitIo.m_pReadBuf;
	}
	virtual CReadBuffer *GetWriteBuf()
	{
		return m_BitIo.m_pWriteBuf;
	}
	virtual BitIo *GetBitIo()
	{
		return &m_BitIo;
	}
	
	virtual void DecodeStart();
	virtual unsigned short DecodeC(size_t &decode_count);
	virtual unsigned short DecodeP(size_t &decode_count);

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

	virtual CReadBuffer *GetReadBuf()
	{
		return m_BitIo.m_pReadBuf;
	}
	virtual CReadBuffer *GetWriteBuf()
	{
		return m_BitIo.m_pWriteBuf;
	}
	virtual BitIo *GetBitIo()
	{
		return &m_BitIo;
	}
	
	virtual void DecodeStart();
	virtual unsigned short DecodeC(size_t &decode_count);
	virtual unsigned short DecodeP(size_t &decode_count);
	
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

	virtual CReadBuffer *GetReadBuf()
	{
		return m_BitIo.m_pReadBuf;
	}
	virtual CReadBuffer *GetWriteBuf()
	{
		return m_BitIo.m_pWriteBuf;
	}
	virtual BitIo *GetBitIo()
	{
		return &m_BitIo;
	}
	
	virtual void DecodeStart();
	virtual unsigned short DecodeC(size_t &decode_count);
	virtual unsigned short DecodeP(size_t &decode_count);
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

	virtual CReadBuffer *GetReadBuf()
	{
		return m_BitIo.m_pReadBuf;
	}
	virtual CReadBuffer *GetWriteBuf()
	{
		return m_BitIo.m_pWriteBuf;
	}
	virtual BitIo *GetBitIo()
	{
		return &m_BitIo;
	}

	virtual void DecodeStart();
	virtual unsigned short DecodeC(size_t &decode_count);
	virtual unsigned short DecodeP(size_t &decode_count);
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
	
	virtual CReadBuffer *GetReadBuf()
	{
		return m_BitIo.m_pReadBuf;
	}
	virtual CReadBuffer *GetWriteBuf()
	{
		return m_BitIo.m_pWriteBuf;
	}
	virtual BitIo *GetBitIo()
	{
		return &m_BitIo;
	}

	virtual void DecodeStart();
	virtual unsigned short DecodeC(size_t &decode_count);
	virtual unsigned short DecodeP(size_t &decode_count);
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
	
	virtual CReadBuffer *GetReadBuf()
	{
		return m_BitIo.m_pReadBuf;
	}
	virtual CReadBuffer *GetWriteBuf()
	{
		return m_BitIo.m_pWriteBuf;
	}
	virtual BitIo *GetBitIo()
	{
		return &m_BitIo;
	}
	
	virtual void DecodeStart();
	virtual unsigned short DecodeC(size_t &decode_count);
	virtual unsigned short DecodeP(size_t &decode_count);
};


#endif // LHDECODER_H
