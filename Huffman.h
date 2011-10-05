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
	
	// simplify code, reusable method
	void bufferSet(const uint16_t c, uint16_t *pBuf, const int size) const
	{
		for (int n = 0; n < size; n++)
		{
			pBuf[n] = c;
		}
	}

	// protected: only derived instance possible
    CHuffman()
		: m_BitIo()
	{}
};


class CHuffmanTree
{
protected:
	
	uint16_t left[2 * NC_LEN - 1];
	uint16_t right[2 * NC_LEN - 1];
	
	uint16_t c_table[4096];   /* decode */
	uint16_t pt_table[256];   /* decode */
	
	// used by the shuffling also..
	// should be in base?
	uint8_t c_len[NC_LEN];
	
public:
    CHuffmanTree()
	{}
	
	void make_table(const int16_t nchar, uint8_t bitlen[], int16_t tablebits, uint16_t table[]);

protected:
	inline void make_table_tree(const int treeLen, const int j, unsigned int &i, uint16_t *pTbl, int &avail);
};


class CDynamicHuffman : public CHuffman, public CHuffmanTree
{
protected:
	// avoid name collisions
	enum tDynamicHuffman
	{
		DYNH_MAXMATCH_LH2 = MAXMATCH,	// -lh2- maxmatch (init only)
		DYNH_NUM_MAX_LH2 = 286,			// -lh2- num max (init)
	
		N_CHAR     = (256 + 60 - THRESHOLD + 1),
		TREESIZE_C = (N_CHAR * 2),
		TREESIZE_P = (128 * 2),
		TREESIZE   = (TREESIZE_C + TREESIZE_P),
		ROOT_C     = 0,
		ROOT_P     = TREESIZE_C
	};

	// handling of these likely break if not 16-bit..
	// -> force on all platforms
	int16_t    child[TREESIZE]; 
	int16_t    parent[TREESIZE];
	int16_t    block[TREESIZE];
	int16_t    edge[TREESIZE];
	int16_t    stock[TREESIZE];
	int16_t    s_node[TREESIZE / 2];   /* Changed N.Watazaki *//*  node[..] -> s_node[..] */
	
	uint16_t freq[TREESIZE];
	
	uint32_t n_max; //
	
	uint16_t total_p;
	int      avail;
	int      m_n1;
	int      most_p;
	int      m_nn;
	uint32_t m_nextcount;

	// shared code called when starting decoding,
	// used from inherited only
	// (only with: -lh1-, -lh2-, -lh3-)
	void init_num_max(const uint32_t num_max)
	{
		n_max = num_max;
	}
	
public:
    CDynamicHuffman()
		: CHuffman()
		, CHuffmanTree()
	    , n_max(0)
		, m_nextcount(0)
	{}

	void decode_start_dyn(const tHuffBits enBit);
	uint16_t decode_c_dyn(); // -lh1-, -lh2-
	uint16_t decode_p_dyn(size_t &decode_count); // -lh2-

protected:
	void start_c_dyn(const uint16_t maxmatch);

	void reconst(int start, int end);
	int swap_inc(int p);
	void swap_inc_Adjust(int &p, int &b);

	void dyn_update_c(int p);
	void dyn_update_p(int p);
	
	void make_new_node(int p);
};

// mess sharing entirely different handlings of 1..3
class CShuffleHuffman : public CDynamicHuffman
{
protected:
	// avoid name collisions
	enum tShuffle
	{
		SHUF_MAXMATCH_LH1 = 60,			// -lh1- maxmatch (init only)
		SHUF_NUM_MAX_LH1 = 314,			// -lh1- num_max (init)
		SHUF_NUM_MAX_LH3 = 286,			// -lh3- num max (init)
		SHUF_NP_LZHUFF1 = (1 << 6),		// -lh1- init value for m_np
		SHUF_NP_LZHUFF3 = (1 << 7),		// -lh3- init value for m_np
		SHUF_NP  = (8 * 1024 / 64),
		SHUF_N1  = 286,                     // alphabet size
		SHUF_EXTRABITS   = 8,               // >= log2(F-THRESHOLD+258-N1) 
		SHUF_BUFBITS     = 16,              // >= log2(MAXBUF)
		SHUF_LENFIELD    = 4               // bit size of length field for tree output
	};

	uint8_t pt_len[PT_LEN_SIZE];
	
	unsigned int m_np;
	
	// was static
	uint16_t m_blocksize; /* decode */

	// these lookup-tables only used by -lh1- and -lh2-
	static const int fixed_method_lh1[16]; // old compatible
	static const int fixed_method_lh3[16]; // 8K buf

	// uses lookup-tables with -lh1- and -lh3-
	void fixed_method_pt_len(const int *tbl);
	
public:
    CShuffleHuffman()
		: CDynamicHuffman()
		, m_blocksize(0)
	{}

	void decode_start_fix(); // -lh1-
	void decode_start_st0(); // -lh3-
	
	uint16_t decode_c_st0(); // -lh3-
	uint16_t decode_p_st0(); // -lh1-, -lh3-
	
protected:
	// used by: decode_c_st0(), decode_p_st0()
	inline void shuf_decode_bitbuf(int &j, const int16_t bitbuf, const int nCount);
	
	void read_tree_c();
	void read_tree_p();
};

class CStaticHuffman : public CHuffman, public CHuffmanTree
{
protected:

	uint8_t pt_len[PT_LEN_SIZE];
	
	int16_t m_blocksize; /* decode */
	
	int m_dict_bit; // set in decode_start_st1()
	unsigned int m_np_dict; // depends on dict bit (16-bit uint only?)
	
	void read_pt_len(int16_t nn, int16_t nbit, int16_t i_special);
	void read_c_len();
	
public:
    CStaticHuffman()
		: CHuffman()
		, CHuffmanTree()
	    , m_blocksize(0)
	{}
	
	uint16_t decode_c_st1();
	uint16_t decode_p_st1();
	
	void decode_start_st1(const tHuffBits enBit);

protected:
	// used by: decode_c_st1(), decode_p_st1(), read_c_len()
	inline void decode_mask_bitbuf(uint16_t &j, const int nCount, const int masksize);
};

#endif // HUFFMAN_H
