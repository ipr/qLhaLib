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
	size_t origsize;
	size_t compsize;


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

public:
    CHuffman()
		: m_BitIo()
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
	
	short make_tree(int nchar, unsigned short *freq, unsigned char *bitlen, unsigned short *code) const;
	
};


/* shuf.c */
#define N1          286             /* alphabet size */
#define N2          (2 * N1 - 1)    /* # of nodes in Huffman tree */
#define EXTRABITS   8               /* >= log2(F-THRESHOLD+258-N1) */
#define BUFBITS     16              /* >= log2(MAXBUF) */
#define LENFIELD    4               /* bit size of length field for tree output */


class CShuffleHuffman : public CHuffman
{
protected:
	const int ciNP  = (8 * 1024 / 64);
	//const int ciNP2 = (ciNP * 2 - 1);

	unsigned int np;
	
	// was static
	unsigned short blocksize; /* decode */

	int fixed[2][16] = {
		{3, 0x01, 0x04, 0x0c, 0x18, 0x30, 0},   /* old compatible */
		{2, 0x01, 0x01, 0x03, 0x06, 0x0D, 0x1F, 0x4E, 0}    /* 8K buf */
	};
	
public:
    CShuffleHuffman()
		: CHuffman()
		, blocksize(0)
	{
	}
	
	void decode_start_st0( /*void*/ );
	void encode_p_st0(unsigned short j);
	void ready_made(int method);
	void encode_start_fix( /*void*/ );
	void read_tree_c( /*void*/ );
	void read_tree_p(/*void*/);
	
	void decode_start_fix(/*void*/);
	
	unsigned short decode_c_st0(/*void*/);
	unsigned short decode_p_st0(/*void*/);
};


/* dhuf.c */
#define N_CHAR      (256 + 60 - THRESHOLD + 1)
#define TREESIZE_C  (N_CHAR * 2)
#define TREESIZE_P  (128 * 2)
#define TREESIZE    (TREESIZE_C + TREESIZE_P)
#define ROOT_C      0
#define ROOT_P      TREESIZE_C


class CDynamicHuffman : public CHuffman
{
protected:
	
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
	int      nn;
	unsigned long nextcount;

	/* from dhuf.c */
	unsigned int n_max;
	
public:
    CDynamicHuffman()
		: CHuffman()
	{
	}
	
	void start_c_dyn( /* void */ );
	void start_p_dyn( /* void */ );	

	void decode_start_dyn( /* void */ );
	void reconst(int start, int end);
	int swap_inc(int p);
	void swap_inc_Adjust(int &p, int &b);

	void update_c(int p);
	void update_p(int p);
	
	void make_new_node(int p);
	
	void encode_c_dyn(unsigned int c);
	unsigned short decode_c_dyn( /* void */ );
	unsigned short decode_p_dyn( /* void */ );
	
	void output_dyn(unsigned int code, unsigned int pos);
	void encode_end_dyn( /* void */ );
	
};


/* huf.c */
#define NP          (MAX_DICBIT + 1)
#define NT          (USHRT_BIT + 3)
#define NC          (UCHAR_MAX + MAXMATCH + 2 - THRESHOLD)

#define PBIT        5       /* smallest integer such that (1 << PBIT) > * NP */
#define TBIT        5       /* smallest integer such that (1 << TBIT) > * NT */
#define CBIT        9       /* smallest integer such that (1 << CBIT) > * NC */

/*      #if NT > NP #define NPT NT #else #define NPT NP #endif  */
#define NPT         0x80


class CStaticHuffman : public CHuffman
{
protected:
	unsigned short left[2 * NC - 1];
	unsigned short right[2 * NC - 1];
	
	unsigned short c_code[NC];      /* encode */
	unsigned short pt_code[NPT];    /* encode */
	
	unsigned short c_table[4096];   /* decode */
	unsigned short pt_table[256];   /* decode */
	
	unsigned short c_freq[2 * NC - 1]; /* encode */
	unsigned short p_freq[2 * NP - 1]; /* encode */
	unsigned short t_freq[2 * NT - 1]; /* encode */
	
	unsigned char  c_len[NC];
	unsigned char  pt_len[NPT];
	
	unsigned char *buf;      /* encode */
	unsigned int bufsiz;     /* encode */
	unsigned short blocksize; /* decode */
	unsigned short output_pos, output_mask; /* encode */
	
	int pbit;
	int np;
	
public:
    CStaticHuffman()
		: CHuffman()
		, blocksize(0)
	{
	}
	
	void count_t_freq(/*void*/);
	void write_pt_len(short n, short nbit, short i_special);
	void write_c_len(/*void*/);
	void encode_c(short c);
	void encode_p(unsigned short  p);
	void send_block( /* void */ );
	
	void output_st1(unsigned short  c, unsigned short  p);
	unsigned char *alloc_buf( /* void */ );
	void encode_start_st1( /* void */ );
	void encode_end_st1( /* void */ );

	void read_pt_len(short nn, short nbit, short i_special);
	void read_c_len( /* void */ );
	unsigned short decode_c_st1( /*void*/ );
	unsigned short decode_p_st1( /* void */ );
	void decode_start_st1( /* void */ );
	
};


#endif // HUFFMAN_H
