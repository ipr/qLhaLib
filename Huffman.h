#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdint.h>
#include <limits.h>

#include "AnsiFile.h"
#include "LhaTypeDefs.h"


class BitIo
{
public:
	unsigned short bitbuf;

	unsigned char subbitbuf;
	unsigned char bitcount;

	/* slide.c */
	int unpackable; // encoding only?
	size_t origsize; // (uncompressed) size of file
	size_t compsize; // compressed size of file


	// helper for accessing same data
	// in decoder and extract-handling
	CReadBuffer *m_pReadBuf;
	CReadBuffer *m_pWriteBuf;
	
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
		: m_pReadBuf(nullptr)
		, m_pWriteBuf(nullptr)
		, bitbuf(0)
		, subbitbuf(0)
	{}
	
};


//class CSlide;
//class CShuffle;

class CHuffman
{
protected:
	BitIo m_BitIo;
	
	// TODO: should use enum-type tHuffBits instead
	// mostly used by encoding?
	// -> not currently
	// note: ambiguous name, should be: m_dictsize ?
	//unsigned short m_dicbit;

	// mostly used by encoding?
	unsigned short maxmatch;
	
	/* from dhuf.c */
	unsigned int n_max;
	
public:
    CHuffman()
		: m_BitIo()
	{}

	// shared code called when starting decoding
	// (used with: -lh1-, -lh2-, -lh3- only)
	void init_decode_start(unsigned int num_max, unsigned short num_maxmatch);
	
	// shared code called when starting encoding
	// (used with: -lh1- only)
	void init_encode_start(unsigned int num_max, unsigned short num_maxmatch);
};


class CHuffmanTree
{
protected:
	// avoid name collisions
	enum tHuffmanTree
	{
		NP         = (MAX_DICBIT + 1),
		NT         = (USHRT_BIT + 3),
		NC         = (UCHAR_MAX + MAXMATCH + 2 - THRESHOLD),
		
		/*      #if NT > NP #define NPT NT #else #define NPT NP #endif  */
		NPT        = 0x80
	};
	
	unsigned short left[2 * NC - 1];
	unsigned short right[2 * NC - 1];
	
	unsigned short c_code[NC];      /* encode */
	unsigned short pt_code[NPT];    /* encode */
	
	unsigned short c_table[4096];   /* decode */
	unsigned short pt_table[256];   /* decode */
	
	unsigned short c_freq[2 * NC - 1]; /* encode */
	unsigned short p_freq[2 * NP - 1]; /* encode */
	unsigned short t_freq[2 * NT - 1]; /* encode */
	
	// these used by the shuffling also..
	// should be in base?
	//
	unsigned char  c_len[NC];
	unsigned char  pt_len[NPT];
	
public:
    CHuffmanTree()
	{}
	
	void make_code(int nchar, 
				   unsigned char  *bitlen, 
				   unsigned short *code,       /* table */
				   unsigned short *leaf_num) const;
	
	void count_leaf(
			int node, /* call with node = root */
			int nchar, 
			unsigned short leaf_num[], 
			int depth) const;
	
	void make_len(int nchar, 
				  unsigned char *bitlen,
				  unsigned short *sort,       /* sorted characters */
				  unsigned short *leaf_num) const;
	
	void downheap(int i, short *heap, size_t heapsize, unsigned short *freq) const;
	
	short make_tree(int nchar, unsigned short *freq, unsigned char *bitlen, unsigned short *code);
	
	void make_table(short nchar, unsigned char bitlen[], short tablebits, unsigned short table[]);
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
	int      n1;
	int      most_p;
	int      m_nn;
	unsigned long nextcount;
	
public:
    CDynamicHuffman()
		: CHuffman()
		, CHuffmanTree()
	{
	}
	
	void start_c_dyn( /* void */ );

	void decode_start_dyn(const tHuffBits enBit);
	void reconst(int start, int end);
	int swap_inc(int p);
	void swap_inc_Adjust(int &p, int &b);

	void update_c(int p);
	void update_p(int p);
	
	void make_new_node(int p);
	
	void encode_c_dyn(unsigned int c);
	unsigned short decode_c_dyn( /* void */ );
	unsigned short decode_p_dyn(size_t &decode_count);
	
	void output_dyn(unsigned int code, unsigned int pos);
	void encode_end_dyn( /* void */ );

	// should be in CShuffleHuffman but called from CDynamicHuffman..
	// -> renamed encode_p_st0() to encode_p_dyn()
	void encode_p_dyn(unsigned short j);
};


class CShuffleHuffman : public CDynamicHuffman
{
protected:
	// avoid name collisions
	enum tShuffle
	{
		SHUF_NP  = (8 * 1024 / 64),
		//SHUF_NP2 = (SHUF_NP * 2 - 1),
		SHUF_N1  = 286,                     // alphabet size
		//SHUF_N2  = (2 * SHUF_N1 - 1),     // # of nodes in Huffman tree 
		SHUF_EXTRABITS   = 8,               // >= log2(F-THRESHOLD+258-N1) 
		SHUF_BUFBITS     = 16,              // >= log2(MAXBUF)
		SHUF_LENFIELD    = 4,               // bit size of length field for tree output
						   
		PBIT       = 5,       /* smallest integer such that (1 << PBIT) > * NP */
		TBIT       = 5,       /* smallest integer such that (1 << TBIT) > * NT */
		CBIT       = 9       /* smallest integer such that (1 << CBIT) > * NC */
	};

	unsigned int m_np;
	
	// was static
	unsigned short m_blocksize; /* decode */

	// only used by ready_made()..
	static const int fixed[2][16];
	
public:
    CShuffleHuffman()
		: CDynamicHuffman()
		, m_blocksize(0)
	{
	}
	
	void ready_made(int method);
	
	void decode_start_st0( /*void*/ );
	void encode_start_fix( /*void*/ );
	void read_tree_c( /*void*/ );
	void read_tree_p(/*void*/);
	
	void decode_start_fix(/*void*/);
	
	unsigned short decode_c_st0(/*void*/);
	unsigned short decode_p_st0(/*void*/);
};


class CStaticHuffman : public CHuffman, public CHuffmanTree
{
protected:
	// avoid name collisions
	enum tStaticHuffman
	{
		PBIT       = 5,       /* smallest integer such that (1 << PBIT) > * NP */
		TBIT       = 5,       /* smallest integer such that (1 << TBIT) > * NT */
		CBIT       = 9       /* smallest integer such that (1 << CBIT) > * NC */
	};

	
	unsigned char *m_pBuf;      /* encode */
	unsigned int bufsiz;     /* encode */
	unsigned short m_blocksize; /* decode */
	unsigned short output_pos; /* encode */
	unsigned short output_mask; /* encode */
	
	int m_pbit;
	int m_np;
	
public:
    CStaticHuffman()
		: CHuffman()
		, CHuffmanTree()
		, m_pBuf(nullptr) // only allocated when encoding?
		, bufsiz(0)
		, m_blocksize(0)
	{
	}
	
	void count_t_freq(/*void*/);
	void write_pt_len(short n, short nbit, short i_special);
	void write_c_len(/*void*/);
	void encode_c(short c);
	void encode_p(unsigned short  p);
	void send_block( /* void */ );
	
	void output_st1(unsigned short  c, unsigned short  p);
	
	void encode_start_st1(const tHuffBits enBit);
	void encode_end_st1( /* void */ );

	void read_pt_len(short nn, short nbit, short i_special);
	void read_c_len( /* void */ );
	unsigned short decode_c_st1( /*void*/ );
	unsigned short decode_p_st1( /* void */ );
	
	void decode_start_st1(const tHuffBits enBit);

protected:
	// used by decode_c_st1() and decode_p_st1()
	inline void decode_st1_mask_bitbuf(unsigned short &j, const int nCount);
	
	// used by decode_start_st1() and encode_start_st1()
	bool SetBitsByDictbit(const tHuffBits enBit);
	
	// used by send_block() and encode_start_st1()
	inline void clear_c_p_freq();

	// used by count_t_freq()
	inline void clear_t_freq();
	
};


#endif // HUFFMAN_H
