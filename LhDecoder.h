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
	// with differences, dynamic huffman

	/*
	// -lh3-, static huffman (0)
	void decode_start_st0();

	// -lh4- .. -lh7-, static huffman (1)
	void decode_start_st1();

	//// decode C
	// -lh1- and -lh2-, dynamic huffman
	unsigned short decode_c_dyn_huf();

	// -lh3-, static huffman (0)
	unsigned short decode_c_st0_huf();

	// -lh4- .. -lh7-, static huffman (1)
	unsigned short decode_c_st1_huf();
	
	//// decode P
	// -lh2-, dynamic huffman
	unsigned short decode_p_dyn_huf();
	
	// -lh1- and -lh3-, static huffman (0)
	unsigned short decode_p_st0_huf();
	
	// -lh4- .. -lh7-, static huffman (1)
	unsigned short decode_p_st1_huf();
	*/

	
protected:
	//BitIo m_BitIo;

	// decode-text
	//unsigned char *m_pDecodeTable;
	unsigned char *m_pText;
	
	// helper for accessing same data
	// in decoder and extract-handling
	CReadBuffer *m_pReadBuf;
	CReadBuffer *m_pWriteBuf; // may grow
	
	/*
	// size of decoded output (should be smaller than buffer)
	size_t m_nDecodedSize;

	// amount of packed data read
	size_t m_nReadPackedSize;
	*/

	// was global..
    unsigned long m_loc;
	
public:
	CLhDecoder(void)
		//: m_BitIo()
		: m_pText(nullptr)
		//, m_pDecodeTable(nullptr)
		, m_pReadBuf(nullptr)
		, m_pWriteBuf(nullptr)
		//, m_nDecodedSize(0)
		//, m_nReadPackedSize(0)
		, m_loc(0)
	{}
	virtual ~CLhDecoder(void)
	{}

	// Create(): only called once by container
	// (optional)
	virtual void CreateDecoder()
	{
		//m_pDecodeTable = new unsigned char[size];
	}
	
	/* // both used by caller..
	void SetTextDictBuf(unsigned char *pText, unsigned char *pDText = nullptr)
	{
		m_pText = pText;
	}
	*/
	
	void SetBuffers(CReadBuffer *pReadBuf, CReadBuffer *pWriteBuf)
	{
		m_pReadBuf = pReadBuf;
		m_pWriteBuf = pWriteBuf;
		
		// temp, make better later..
		//m_BitIo.m_pReadBuf = pReadBuf;
		//m_BitIo.m_pWriteBuf = pWriteBuf;
	}
	
	virtual void DecodeStart() = 0;
	virtual unsigned short DecodeC() = 0;
	virtual unsigned short DecodeP() = 0;
	

	/*
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
	*/
	
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
};

//////// decoders

// (note: -lh0-, -lhd- and -lz4- are "store only", no compression)

// -lh1-
// (dynamic huffman)
class CLhDecodeLh1 : public CLhDecoder, protected CShuffleHuffman, protected CDynamicHuffman
{
public:
	CLhDecodeLh1(void)
		: CLhDecoder()
		, CShuffleHuffman()
		, CDynamicHuffman()
	{}
	virtual ~CLhDecodeLh1(void)
	{}
	
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
	
	virtual void DecodeStart();
	virtual unsigned short DecodeC();
	virtual unsigned short DecodeP();
};


#endif // LHDECODER_H
