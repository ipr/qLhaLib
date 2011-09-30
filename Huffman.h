#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdint.h>
#include <limits.h>

#include "AnsiFile.h"
#include "LhaTypeDefs.h"


// all file-IO was through functions in bitio,
// which is now this class taking buffers only:
// read/write in larger chunks instead of per-character now.
//
class BitIo
{
public:
	uint16_t bitbuf;
	uint8_t subbitbuf;
	uint8_t bitcount;

	size_t origsize; // (uncompressed) size of file
	size_t compsize; // compressed size of file


	// helper for accessing same data
	// in decoder and extract-handling
	CReadBuffer *m_pReadBuf;
	CReadBuffer *m_pWriteBuf;
	
	uint16_t peekbits(const uint8_t count)
	{
		return (bitbuf >> ((sizeof(bitbuf)*8) - count));
	}

	/* Shift bitbuf n bits left, read n bits */
	void fillbuf(const uint8_t count)
	{
		uint8_t n = count;
		while (n > bitcount) 
		{
	        n -= bitcount;
	        bitbuf = (bitbuf << bitcount) + (subbitbuf >> (CHAR_BIT - bitcount));
	        if (compsize != 0) 
			{
	            compsize--;
				subbitbuf = m_pReadBuf->GetNextByte();
	        }
	        else
			{
	            subbitbuf = 0;
			}
	        bitcount = CHAR_BIT;
	    }
	    bitcount -= n;
	    bitbuf = (bitbuf << n) + (subbitbuf >> (CHAR_BIT - n));
	    subbitbuf <<= n;
	}
	
	uint16_t getbits(const uint8_t count)
	{
		uint16_t x = (bitbuf >> (2 * CHAR_BIT - count));
		fillbuf(count);
		return x;
	}
	
	void init_getbits()
	{
		bitbuf = 0;
		subbitbuf = 0;
		bitcount = 0;
		fillbuf(2 * CHAR_BIT);
	}

public:
    BitIo()
		: m_pReadBuf(nullptr)
		, m_pWriteBuf(nullptr)
		, bitbuf(0)
		, subbitbuf(0)
	{}
	
};


// some various decoding-values used everywhere in decoding..
//
enum tDecodingLimits
{
	MAXMATCH           = 256, /* formerly F (not more than UCHAR_MAX + 1) */
	THRESHOLD          = 3,   /* choose optimal value */

	USHRT_BIT          = 16,  /* (CHAR_BIT * sizeof(ushort)) */
	
	NP_LEN         = (MAX_DICBIT + 1),
	NT_LEN         = (USHRT_BIT + 3),
	NC_LEN         = (UCHAR_MAX + MAXMATCH + 2 - THRESHOLD),
	
	CBIT       = 9,       /* smallest integer such that (1 << CBIT) > * NC */
	
	/*      #if NT > NP #define NPT NT #else #define NPT NP #endif  */
	PT_LEN_SIZE        = 0x80
};


class CHuffman
{
protected:
	BitIo m_BitIo;
	
	unsigned short m_maxmatch;
	unsigned int n_max;
	
public:
    CHuffman()
		: m_BitIo()
	    , m_maxmatch(0)
	    , n_max(0)
	{}

	// shared code called when starting decoding
	// (used with: -lh1-, -lh2-, -lh3- only)
	void init_decode_start(unsigned int num_max, unsigned short maxmatch)
	{
		n_max = num_max;
		m_maxmatch = maxmatch;
	}
};


class CHuffmanTree
{
protected:
	
	unsigned short left[2 * NC_LEN - 1];
	unsigned short right[2 * NC_LEN - 1];
	
	unsigned short c_table[4096];   /* decode */
	unsigned short pt_table[256];   /* decode */
	
	// used by the shuffling also..
	// should be in base?
	unsigned char  c_len[NC_LEN];
	
public:
    CHuffmanTree()
	{}
	
	void make_table(short nchar, unsigned char bitlen[], short tablebits, unsigned short table[]);

protected:
	inline void make_table_tree(int nLen, const int j, unsigned int &i, unsigned short *pTbl, int &avail);
};


class CDynamicHuffman : public CHuffman, public CHuffmanTree
{
protected:
	// avoid name collisions
	enum tDynamicHuffman
	{
		N_CHAR     = (256 + 60 - THRESHOLD + 1),
		TREESIZE_C = (N_CHAR * 2),
		TREESIZE_P = (128 * 2),
		TREESIZE   = (TREESIZE_C + TREESIZE_P),
		ROOT_C     = 0,
		ROOT_P     = TREESIZE_C
	};
	
	short    child[TREESIZE]; 
	short    parent[TREESIZE];
	short    block[TREESIZE];
	short    edge[TREESIZE];
	short    stock[TREESIZE];
	short    s_node[TREESIZE / 2];   /* Changed N.Watazaki */
	/*  node[..] -> s_node[..] */
	
	unsigned short freq[TREESIZE];
	
	unsigned short total_p;
	int      avail;
	int      m_n1;
	int      most_p;
	int      m_nn;
	unsigned long nextcount;
	
public:
    CDynamicHuffman()
		: CHuffman()
		, CHuffmanTree()
	{}
	
	void start_c_dyn();

	void decode_start_dyn(const tHuffBits enBit);
	void reconst(int start, int end);
	int swap_inc(int p);
	void swap_inc_Adjust(int &p, int &b);

	void dyn_update_c(int p);
	void dyn_update_p(int p);
	
	void make_new_node(int p);
	
	unsigned short decode_c_dyn();
	unsigned short decode_p_dyn(size_t &decode_count);
	
};

// mess sharing entirely different handlings of 1..3
class CShuffleHuffman : public CDynamicHuffman
{
protected:
	// avoid name collisions
	enum tShuffle
	{
		SHUF_NP_LZHUFF1 = (1 << 6), // -lh1- init value for m_np
		SHUF_NP_LZHUFF3 = (1 << 7), // -lh3- init value for m_np
		SHUF_NP  = (8 * 1024 / 64),
		SHUF_N1  = 286,                     // alphabet size
		SHUF_EXTRABITS   = 8,               // >= log2(F-THRESHOLD+258-N1) 
		SHUF_BUFBITS     = 16,              // >= log2(MAXBUF)
		SHUF_LENFIELD    = 4               // bit size of length field for tree output
	};

	unsigned char  pt_len[PT_LEN_SIZE];
	
	unsigned int m_np;
	
	// was static
	unsigned short m_blocksize; /* decode */

	// these lookup-tables only used by -lh1- and -lh2-
	static const int fixed_method_lh1[16]; // old compatible
	static const int fixed_method_lh3[16]; // 8K buf
	
public:
    CShuffleHuffman()
		: CDynamicHuffman()
		, m_blocksize(0)
	{}

	// uses lookup-tables with -lh1- and -lh3-
	//void fixed_method_pt_len(int method);
	void fixed_method_pt_len(const int *tbl);

	void decode_start_fix();
	
	void decode_start_st0();
	void read_tree_c();
	void read_tree_p();
	
	unsigned short decode_c_st0();
	unsigned short decode_p_st0();
};

class CStaticHuffman : public CHuffman, public CHuffmanTree
{
protected:

	unsigned short m_blocksize; /* decode */
	
	int m_dict_bit; // set in decode_start_st1()
	unsigned int m_np_dict; // depends on dict bit

	unsigned char pt_len[PT_LEN_SIZE];
	
public:
    CStaticHuffman()
		: CHuffman()
		, CHuffmanTree()
	    , m_blocksize(0)
	{}
	
	void read_pt_len(short nn, short nbit, short i_special);
	void read_c_len();
	unsigned short decode_c_st1();
	unsigned short decode_p_st1();
	
	void decode_start_st1(const tHuffBits enBit);

protected:
	// used by decode_c_st1() and decode_p_st1()
	inline void decode_st1_mask_bitbuf(unsigned short &j, const int nCount);
	
};

#endif // HUFFMAN_H
