#include "Huffman.h"

#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <stdlib.h>


/* Shift bitbuf n bits left, read n bits */
void BitIo::fillbuf(unsigned char n)
{
    while (n > bitcount) 
	{
        n -= bitcount;
        bitbuf = (bitbuf << bitcount) + (subbitbuf >> (CHAR_BIT - bitcount));
        if (compsize != 0) 
		{
            compsize--;
			subbitbuf = m_pReadBuf->GetNext();
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

/* Write leftmost n bits of x */
void BitIo::putcode(unsigned char n, unsigned short x)
{
    while (n >= bitcount) 
	{
        n -= bitcount;
        subbitbuf += x >> (USHRT_BIT - bitcount);
        x <<= bitcount;
        if (compsize < origsize) 
		{
			m_pWriteBuf->SetNext(subbitbuf);
            compsize++;
        }
        else
		{
            unpackable = 1;
		}
        subbitbuf = 0;
        bitcount = CHAR_BIT;
    }
    subbitbuf += x >> (USHRT_BIT - bitcount);
    bitcount -= n;
}

///////////

// base
CHuffman::CHuffman()
	: m_BitIo()
{
}

/* ------------------------------------------------------------------------ */
/* LHa for UNIX                                                             */
/*              maketree.c -- make Huffman tree                             */
/*                                                                          */
/*      Modified                Nobutaka Watazaki                           */
/*                                                                          */
/*  Ver. 1.14   Source All chagned              1995.01.14  N.Watazaki      */
/* ------------------------------------------------------------------------ */

void CHuffman::make_code(int nchar, 
			   unsigned char  *bitlen, 
			   unsigned short *code,       /* table */
			   unsigned short *leaf_num) const
{
    unsigned short  weight[17]; /* 0x10000ul >> bitlen */
    unsigned short  start[17];  /* start code */
    int i = 1;
    int c = 0;
    unsigned short total = 0;
	
    for (i = 1; i <= 16; i++) 
	{
        start[i] = total;
        weight[i] = 1 << (16 - i);
        total += weight[i] * leaf_num[i];
    }
	
    for (c = 0; c < nchar; c++) 
	{
        i = bitlen[c];
        code[c] = start[i];
        start[i] += weight[i];
    }
}

void CHuffman::count_leaf(int node, /* call with node = root */
						  int nchar, 
						  unsigned short leaf_num[], 
						  int depth) const
{
    if (node < nchar)
	{
        leaf_num[depth < 16 ? depth : 16]++;
	}
    else 
	{
        count_leaf(left[node], nchar, leaf_num, depth + 1);
        count_leaf(right[node], nchar, leaf_num, depth + 1);
    }
}

void CHuffman::make_len(int nchar, 
			  unsigned char *bitlen,
			  unsigned short *sort,       /* sorted characters */
			  unsigned short *leaf_num) const
{
    int i = 0;

    unsigned int cum = 0;
    for (i = 16; i > 0; i--) 
	{
        cum += leaf_num[i] << (16 - i);
    }
	
#if (UINT_MAX != 0xffff)
    cum &= 0xffff;
#endif
	
    /* adjust len */
    if (cum) 
	{
        leaf_num[16] -= cum; /* always leaf_num[16] > cum */
        do 
		{
            for (i = 15; i > 0; i--) 
			{
                if (leaf_num[i]) 
				{
                    leaf_num[i]--;
                    leaf_num[i + 1] += 2;
                    break;
                }
            }
        } while (--cum);
    }
	
    /* make len */
    for (i = 16; i > 0; i--) 
	{
        int k = leaf_num[i];
        while (k > 0) 
		{
            bitlen[*sort++] = i;
            k--;
        }
    }
}

/* priority queue; send i-th entry down heap */
void CHuffman::downheap(int i, short *heap, size_t heapsize, unsigned short *freq) const
{
    short j = 0;
    short k = heap[i];
    while ((j = 2 * i) <= heapsize) 
	{
        if (j < heapsize && freq[heap[j]] > freq[heap[j + 1]])
		{
            j++;
		}
        if (freq[k] <= freq[heap[j]])
		{
            break;
		}
        heap[i] = heap[j];
        i = j;
    }
    heap[i] = k;
}

/* make tree, calculate bitlen[], return root */
short CHuffman::make_tree(int nchar, unsigned short *freq, unsigned char *bitlen, unsigned short *code) const
{
    short i, j, avail, root;
    unsigned short *sort;

    short heap[NC + 1];       /* NC >= nchar */
    size_t heapsize;

    avail = nchar;
    heapsize = 0;
    heap[1] = 0;
    for (i = 0; i < nchar; i++) 
	{
        bitlen[i] = 0;
        if (freq[i])
		{
            heap[++heapsize] = i;
		}
    }
    if (heapsize < 2) 
	{
        code[heap[1]] = 0;
        return heap[1];
    }

    /* make priority queue */
    for (i = heapsize / 2; i >= 1; i--)
	{
        downheap(i, heap, heapsize, freq);
	}

    /* make huffman tree */
    sort = code;
    do {            /* while queue has at least two entries */
        i = heap[1];    /* take out least-freq entry */
        if (i < nchar)
		{
            *sort++ = i;
		}
        heap[1] = heap[heapsize--];
        downheap(1, heap, heapsize, freq);
        j = heap[1];    /* next least-freq entry */
        if (j < nchar)
		{
            *sort++ = j;
		}
        root = avail++;    /* generate new node */
        freq[root] = freq[i] + freq[j];
        heap[1] = root;
        downheap(1, heap, heapsize, freq);    /* put into queue */
        left[root] = i;
        right[root] = j;
    } while (heapsize > 1);

    {
        unsigned short leaf_num[17];

        /* make leaf_num */
        memset(leaf_num, 0, sizeof(leaf_num));
        count_leaf(root, nchar, leaf_num, 0);

        /* make bitlen */
        make_len(nchar, bitlen, code, leaf_num);

        /* make code table */
        make_code(nchar, bitlen, code, leaf_num);
    }

    return root;
}


///


CShuffleHuffman::CShuffleHuffman()
{
}


/* ------------------------------------------------------------------------ */
/* LHa for UNIX                                                             */
/*              shuf.c -- extract static Huffman coding                     */
/*                                                                          */
/*      Modified                Nobutaka Watazaki                           */
/*                                                                          */
/*  Ver. 1.14   Source All chagned              1995.01.14  N.Watazaki      */
/* ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
/* lh3 */
void CShuffleHuffman::decode_start_st0( /*void*/ )
{
    n_max = 286;
    maxmatch = MAXMATCH;
    m_BitIo.init_getbits();
    //init_code_cache();
    np = 1 << (LZHUFF3_DICBIT - 6);
}

/* ------------------------------------------------------------------------ */
void CShuffleHuffman::encode_p_st0(unsigned short  j)
{
    unsigned short i = j >> 6;
    putcode(pt_len[i], pt_code[i]);
    putbits(6, j & 0x3f);
}

/* ------------------------------------------------------------------------ */
void CShuffleHuffman::ready_made(int method)
{
    int             i, j;
    unsigned int    code, weight;
    int            *tbl;

    tbl = fixed[method];
    j = *tbl++;
    weight = 1 << (16 - j);
    code = 0;
    for (i = 0; i < np; i++) {
        while (*tbl == i) {
            j++;
            tbl++;
            weight >>= 1;
        }
        pt_len[i] = j;
        pt_code[i] = code;
        code += weight;
    }
}

/* ------------------------------------------------------------------------ */
/* lh1 */
void CShuffleHuffman::encode_start_fix( /*void*/ )
{
    n_max = 314;
    maxmatch = 60;
    np = 1 << (12 - 6);
    init_putbits();
    //init_code_cache();
    start_c_dyn();
    ready_made(0);
}

/* ------------------------------------------------------------------------ */
void CShuffleHuffman::read_tree_c( /*void*/ )
{
	/* read tree from file */
    int             i, c;

    i = 0;
    while (i < N1) {
        if (getbits(1))
            c_len[i] = getbits(LENFIELD) + 1;
        else
            c_len[i] = 0;
        if (++i == 3 && c_len[0] == 1 && c_len[1] == 1 && c_len[2] == 1) {
            c = getbits(CBIT);
            for (i = 0; i < N1; i++)
                c_len[i] = 0;
            for (i = 0; i < 4096; i++)
                c_table[i] = c;
            return;
        }
    }
    make_table(N1, c_len, 12, c_table);
}

/* ------------------------------------------------------------------------ */
void CShuffleHuffman::read_tree_p(/*void*/)
{
	/* read tree from file */
    int             i, c;

    i = 0;
    while (i < ciNP) 
	{
        pt_len[i] = getbits(LENFIELD);
        if (++i == 3 && pt_len[0] == 1 && pt_len[1] == 1 && pt_len[2] == 1) {
            c = getbits(LZHUFF3_DICBIT - 6);
            for (i = 0; i < ciNP; i++)
                pt_len[i] = 0;
            for (i = 0; i < 256; i++)
                pt_table[i] = c;
            return;
        }
    }
}

/* ------------------------------------------------------------------------ */
/* lh1 */
void CShuffleHuffman::decode_start_fix(/*void*/)
{
    n_max = 314;
    maxmatch = 60;
    init_getbits();
    //init_code_cache();
    np = 1 << (LZHUFF1_DICBIT - 6);
    start_c_dyn();
    ready_made(0);
    make_table(np, pt_len, 8, pt_table);
}

/* ------------------------------------------------------------------------ */
/* lh3 */
unsigned short CShuffleHuffman::decode_c_st0(/*void*/)
{
    int             i, j;
    unsigned short blocksize = 0;

    if (blocksize == 0) {   /* read block head */
        blocksize = getbits(BUFBITS);   /* read block blocksize */
        read_tree_c();
        if (getbits(1)) 
		{
            read_tree_p();
        }
        else 
		{
            ready_made(1);
        }
        make_table(ciNP, pt_len, 8, pt_table);
    }
    blocksize--;
    j = c_table[peekbits(12)];
    if (j < N1)
	{
        m_BitIo.fillbuf(c_len[j]);
	}
    else 
	{
        m_BitIo.fillbuf(12);
        i = bitbuf;
        do 
		{
            if ((short) i < 0)
			{
                j = right[j];
			}
            else
			{
                j = left[j];
			}
            i <<= 1;
        } while (j >= N1);
        m_BitIo.fillbuf(c_len[j] - 12);
    }
    if (j == N1 - 1)
	{
        j += getbits(EXTRABITS);
	}
    return j;
}

/* ------------------------------------------------------------------------ */
/* lh1, 3 */
unsigned short CShuffleHuffman::decode_p_st0(/*void*/)
{
    int j = pt_table[peekbits(8)];
    if (j < np) 
	{
        m_BitIo.fillbuf(pt_len[j]);
    }
    else 
	{
        m_BitIo.fillbuf(8);
        int i = bitbuf;
        do {
            if ((short) i < 0)
			{
                j = right[j];
			}
            else
			{
                j = left[j];
			}
            i <<= 1;
        } while (j >= np);
        m_BitIo.fillbuf(pt_len[j] - 8);
    }
    return (j << 6) + getbits(6);
}

///


// dynamic
CDynamicHuffman::CDynamicHuffman()
{
}

/* ------------------------------------------------------------------------ */
/* LHa for UNIX                                                             */
/*              dhuf.c -- Dynamic Hufffman routine                          */
/*                                                                          */
/*      Modified                H.Yoshizaki                                 */
/*                                                                          */
/*  Ver. 1.14   Source All chagned              1995.01.14  N.Watazaki      */
/* ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

void CDynamicHuffman::start_c_dyn( /* void */ )
{
    int             i, j, f;

    n1 = (n_max >= 256 + maxmatch - THRESHOLD + 1) ? 512 : n_max - 1;
    for (i = 0; i < TREESIZE_C; i++) 
	{
        stock[i] = i;
        block[i] = 0;
    }
    for (i = 0, j = n_max * 2 - 2; i < n_max; i++, j--) 
	{
        freq[j] = 1;
        child[j] = ~i;
        s_node[i] = j;
        block[j] = 1;
    }
    avail = 2;
    edge[1] = n_max - 1;
    i = n_max * 2 - 2;
    while (j >= 0) 
	{
        f = freq[j] = freq[i] + freq[i - 1];
        child[j] = i;
        parent[i] = parent[i - 1] = j;
        if (f == freq[j + 1]) {
            edge[block[j] = block[j + 1]] = j;
        }
        else 
		{
            edge[block[j] = stock[avail++]] = j;
        }
        i -= 2;
        j--;
    }
}

/* ------------------------------------------------------------------------ */
void CDynamicHuffman::start_p_dyn( /* void */ )
{
    freq[ROOT_P] = 1;
    child[ROOT_P] = ~(N_CHAR);
    s_node[N_CHAR] = ROOT_P;
    edge[block[ROOT_P] = stock[avail++]] = ROOT_P;
    most_p = ROOT_P;
    total_p = 0;
    nn = 1 << dicbit;
    nextcount = 64;
}

/* ------------------------------------------------------------------------ */
/* lh2 */
void CDynamicHuffman::decode_start_dyn( /* void */ )
{
    n_max = 286;
    maxmatch = MAXMATCH;
    init_getbits();
    //init_code_cache();
    start_c_dyn();
    start_p_dyn();
}

/* ------------------------------------------------------------------------ */
void CDynamicHuffman::reconst(int start, int end)
{
    int             i, j, k, l, b;
    unsigned int    f, g;

    for (i = j = start; i < end; i++) 
	{
        if ((k = child[i]) < 0) 
		{
            freq[j] = (freq[i] + 1) / 2;
            child[j] = k;
            j++;
        }
        if (edge[b = block[i]] == i) 
		{
            stock[--avail] = b;
        }
    }
    j--;
    i = end - 1;
    l = end - 2;
    while (i >= start) 
	{
        while (i >= l) 
		{
            freq[i] = freq[j];
            child[i] = child[j];
            i--, j--;
        }
        f = freq[l] + freq[l + 1];
        for (k = start; f < freq[k]; k++);
        while (j >= k) 
		{
            freq[i] = freq[j];
            child[i] = child[j];
            i--, j--;
        }
        freq[i] = f;
        child[i] = l + 1;
        i--;
        l -= 2;
    }
    f = 0;
    for (i = start; i < end; i++) 
	{
        if ((j = child[i]) < 0)
		{
            s_node[~j] = i;
		}
        else
		{
            parent[j] = parent[j - 1] = i;
		}
        if ((g = freq[i]) == f) 
		{
            block[i] = b;
        }
        else 
		{
            edge[b = block[i] = stock[avail++]] = i;
            f = g;
        }
    }
}

/* ------------------------------------------------------------------------ */
int CDynamicHuffman::swap_inc(int p)
{
    int  q, r, s;

    int b = block[p];
	int q = edge[b]; 
    if (q != p) /* swap for leader */
	{
        r = child[p];
        s = child[q];
        child[p] = s;
        child[q] = r;
		
        if (r >= 0)
		{
            parent[r] = parent[r - 1] = q;
		}
        else
		{
            s_node[~r] = q;
		}
		
        if (s >= 0)
		{
            parent[s] = parent[s - 1] = p;
		}
        else
		{
            s_node[~s] = p;
		}
        p = q;
		
		swap_inc_Adjust(p, b);
    }
    else if (b == block[p + 1]) 
	{
		swap_inc_Adjust(p, b);
    }
    else if (++freq[p] == freq[p - 1]) 
	{
        stock[--avail] = b; /* delete block */
        block[p] = block[p - 1];
    }
    return parent[p];
}

void CDynamicHuffman::swap_inc_Adjust(int &p, int &b)
{
	edge[b]++;
	if (++freq[p] == freq[p - 1]) 
	{
		block[p] = block[p - 1];
	}
	else 
	{
		edge[block[p] = stock[avail++]] = p;    /* create block */
	}
}

/* ------------------------------------------------------------------------ */
void CDynamicHuffman::update_c(int p)
{
    if (freq[ROOT_C] == 0x8000) 
	{
        reconst(0, n_max * 2 - 1);
    }
    freq[ROOT_C]++;

    int q = s_node[p];
    do 
	{
        q = swap_inc(q);
    } while (q != ROOT_C);
}

/* ------------------------------------------------------------------------ */
void CDynamicHuffman::update_p(int p)
{
    if (total_p == 0x8000) 
	{
        reconst(ROOT_P, most_p + 1);
        total_p = freq[ROOT_P];
        freq[ROOT_P] = 0xffff;
    }
	
    int q = s_node[p + N_CHAR];
    while (q != ROOT_P) 
	{
        q = swap_inc(q);
    }
    total_p++;
}

/* ------------------------------------------------------------------------ */
void CDynamicHuffman::make_new_node(int p)
{
    int             q, r;

    r = most_p + 1;
    q = r + 1;
    s_node[~(child[r] = child[most_p])] = r;
    child[q] = ~(p + N_CHAR);
    child[most_p] = q;
    freq[r] = freq[most_p];
    freq[q] = 0;
    block[r] = block[most_p];
    if (most_p == ROOT_P) 
	{
        freq[ROOT_P] = 0xffff;
        edge[block[ROOT_P]]++;
    }
    parent[r] = parent[q] = most_p;
    edge[block[q] = stock[avail++]] = s_node[p + N_CHAR] = most_p = q;
    update_p(p);
}

/* ------------------------------------------------------------------------ */
void CDynamicHuffman::encode_c_dyn(unsigned int c)
{
    unsigned int    bits;
    int             p, d, cnt;

    d = c - n1;
    if (d >= 0) 
	{
        c = n1;
    }
    cnt = bits = 0;
    p = s_node[c];
    do 
	{
        bits >>= 1;
        if (p & 1) 
		{
            bits |= 0x80000000L;
        }
        cnt++;
    } while ((p = parent[p]) != ROOT_C);
    
	if (cnt <= 16) 
	{
        putcode(cnt, bits >> 16);
    } 
	else 
	{
        putcode(16, bits >> 16);
        putbits(cnt - 16, bits);
    }
	
    if (d >= 0)
	{
        putbits(8, d);
	}
    update_c(c);
}

/* ------------------------------------------------------------------------ */
/* lh1, 2 */
unsigned short CDynamicHuffman::decode_c_dyn( /* void */ )
{
    int             c;
    short           buf, cnt;

    c = child[ROOT_C];
    buf = bitbuf;
    cnt = 0;
    do {
        c = child[c - (buf < 0)];
        buf <<= 1;
        if (++cnt == 16) {
            m_BitIo.fillbuf(16);
            buf = bitbuf;
            cnt = 0;
        }
    } while (c > 0);
    m_BitIo.fillbuf(cnt);
    c = ~c;
    update_c(c);
    if (c == n1)
	{
        c += getbits(8);
	}
    return c;
}

/* ------------------------------------------------------------------------ */
/* lh2 */
unsigned short CDynamicHuffman::decode_p_dyn( /* void */ )
{
    int             c;
    short           buf, cnt;

    while (decode_count > nextcount) 
	{
        make_new_node(nextcount / 64);
        if ((nextcount += 64) >= nn)
		{
            nextcount = 0xffffffff;
		}
    }
    c = child[ROOT_P];
    buf = bitbuf;
    cnt = 0;
    while (c > 0) 
	{
        c = child[c - (buf < 0)];
        buf <<= 1;
        if (++cnt == 16) 
		{
            m_BitIo.fillbuf(16);
            buf = bitbuf;
            cnt = 0;
        }
    }
    m_BitIo.fillbuf(cnt);
    c = (~c) - N_CHAR;
    update_p(c);

    return (c << 6) + getbits(6);
}

/* ------------------------------------------------------------------------ */
/* lh1 */
void CDynamicHuffman::output_dyn(unsigned int code, unsigned int pos)
{
    encode_c_dyn(code);
    if (code >= 0x100) 
	{
        encode_p_st0(pos);
    }
}

/* ------------------------------------------------------------------------ */
/* lh1 */
void CDynamicHuffman::encode_end_dyn( /* void */ )
{
    putcode(7, 0);
}


///

// static
CStaticHuffman::CStaticHuffman()
{
}

/* ------------------------------------------------------------------------ */
/* LHa for UNIX                                                             */
/*              huf.c -- new static Huffman                                 */
/*                                                                          */
/*      Modified                Nobutaka Watazaki                           */
/*                                                                          */
/*  Ver. 1.14   Source All chagned              1995.01.14  N.Watazaki      */
/*  Ver. 1.14i  Support LH7 & Bug Fixed         2000.10. 6  t.okamoto       */
/* ------------------------------------------------------------------------ */


/* ------------------------------------------------------------------------ */
/*                              Encording                                   */
/* ------------------------------------------------------------------------ */
void CStaticHuffman::count_t_freq(/*void*/)
{
    short           i, k, n, count;

    for (i = 0; i < NT; i++)
	{
        t_freq[i] = 0;
	}
    n = NC;
    while (n > 0 && c_len[n - 1] == 0)
	{
        n--;
	}
    i = 0;
    while (i < n) 
	{
        k = c_len[i++];
        if (k == 0) 
		{
            count = 1;
            while (i < n && c_len[i] == 0) 
			{
                i++;
                count++;
            }
            if (count <= 2)
			{
                t_freq[0] += count;
			}
            else if (count <= 18)
			{
                t_freq[1]++;
			}
            else if (count == 19) 
			{
                t_freq[0]++;
                t_freq[1]++;
            }
            else
			{
                t_freq[2]++;
			}
        } 
		else
		{
            t_freq[k + 2]++;
		}
    }
}

/* ------------------------------------------------------------------------ */
void CStaticHuffman::write_pt_len(short n, short nbit, short i_special)
{
    short           i, k;

    while (n > 0 && pt_len[n - 1] == 0)
	{
        n--;
	}
    putbits(nbit, n);
    i = 0;
    while (i < n) 
	{
        k = pt_len[i++];
        if (k <= 6)
		{
            putbits(3, k);
		}
        else
		{
            /* k=7 -> 1110  k=8 -> 11110  k=9 -> 111110 ... */
            putbits(k - 3, USHRT_MAX << 1);
		}
        if (i == i_special) 
		{
            while (i < 6 && pt_len[i] == 0)
			{
                i++;
			}
            putbits(2, i - 3);
        }
    }
}

/* ------------------------------------------------------------------------ */
void CStaticHuffman::write_c_len(/*void*/)
{
    short           i, k, n, count;

    n = NC;
    while (n > 0 && c_len[n - 1] == 0)
	{
        n--;
	}
    putbits(CBIT, n);
    i = 0;
    while (i < n) 
	{
        k = c_len[i++];
        if (k == 0) 
		{
            count = 1;
            while (i < n && c_len[i] == 0) 
			{
                i++;
                count++;
            }
            if (count <= 2) 
			{
                for (k = 0; k < count; k++)
				{
                    putcode(pt_len[0], pt_code[0]);
				}
            }
            else if (count <= 18) 
			{
                putcode(pt_len[1], pt_code[1]);
                putbits(4, count - 3);
            }
            else if (count == 19) 
			{
                putcode(pt_len[0], pt_code[0]);
                putcode(pt_len[1], pt_code[1]);
                putbits(4, 15);
            }
            else 
			{
                putcode(pt_len[2], pt_code[2]);
                putbits(CBIT, count - 20);
            }
        }
        else
		{
            putcode(pt_len[k + 2], pt_code[k + 2]);
		}
    }
}

/* ------------------------------------------------------------------------ */
void CStaticHuffman::encode_c(short c)
{
    putcode(c_len[c], c_code[c]);
}

/* ------------------------------------------------------------------------ */
void CStaticHuffman::encode_p(unsigned short  p)
{
    unsigned short  c, q;

    c = 0;
    q = p;
    while (q) 
	{
        q >>= 1;
        c++;
    }
    putcode(pt_len[c], pt_code[c]);
    if (c > 1)
	{
        putbits(c - 1, p);
	}
}

/* ------------------------------------------------------------------------ */
void CStaticHuffman::send_block( /* void */ )
{
    unsigned char   flags;
    unsigned short  i, k, root, pos, size;

    root = make_tree(NC, c_freq, c_len, c_code);
    size = c_freq[root];
    putbits(16, size);
    if (root >= NC) 
	{
        count_t_freq();
        root = make_tree(NT, t_freq, pt_len, pt_code);
        if (root >= NT) 
		{
            write_pt_len(NT, TBIT, 3);
        } 
		else 
		{
            putbits(TBIT, 0);
            putbits(TBIT, root);
        }
        write_c_len();
    } 
	else 
	{
        putbits(TBIT, 0);
        putbits(TBIT, 0);
        putbits(CBIT, 0);
        putbits(CBIT, root);
    }
    root = make_tree(np, p_freq, pt_len, pt_code);
    if (root >= np) 
	{
        write_pt_len(np, pbit, -1);
    }
    else 
	{
        putbits(pbit, 0);
        putbits(pbit, root);
    }
    pos = 0;
    for (i = 0; i < size; i++) 
	{
        if (i % CHAR_BIT == 0)
		{
            flags = buf[pos++];
		}
        else
		{
            flags <<= 1;
		}
        if (flags & (1 << (CHAR_BIT - 1))) 
		{
            encode_c(buf[pos++] + (1 << CHAR_BIT));
            k = buf[pos++] << CHAR_BIT;
            k += buf[pos++];
            encode_p(k);
        } 
		else
		{
            encode_c(buf[pos++]);
		}
        if (unpackable)
		{
            return;
		}
    }
	
	// why not use memset() here..
	// it's not overlapping or anything..
    for (i = 0; i < NC; i++)
	{
        c_freq[i] = 0;
	}
    for (i = 0; i < np; i++)
	{
        p_freq[i] = 0;
	}
}

/* ------------------------------------------------------------------------ */
/* lh4, 5, 6, 7 */
void CStaticHuffman::output_st1(unsigned short  c, unsigned short  p)
{
    unsigned short cpos;

    output_mask >>= 1;
    if (output_mask == 0) 
	{
        output_mask = 1 << (CHAR_BIT - 1);
        if (output_pos >= bufsiz - 3 * CHAR_BIT) 
		{
            send_block();
            if (unpackable)
			{
                return;
			}
            output_pos = 0;
        }
        cpos = output_pos++;
        buf[cpos] = 0;
    }
	
    buf[output_pos++] = (unsigned char) c;
    c_freq[c]++;
    if (c >= (1 << CHAR_BIT)) 
	{
        buf[cpos] |= output_mask;
        buf[output_pos++] = (unsigned char) (p >> CHAR_BIT);
        buf[output_pos++] = (unsigned char) p;
        c = 0;
        while (p) 
		{
            p >>= 1;
            c++;
        }
        p_freq[c]++;
    }
}

/* ------------------------------------------------------------------------ */
// used by slide.c, defined in huf.c ..
unsigned char *CStaticHuffman::alloc_buf( /* void */ )
{
    bufsiz = 16 * 1024 *2;  /* 65408U; */ /* t.okamoto */
	
	/* ugh.. if there's no 64K RAM we are dead anyway.. remove this.. */
    while ((buf = (unsigned char *) malloc(bufsiz)) == NULL) 
	{
        bufsiz = (bufsiz / 10) * 9;
        if (bufsiz < 4 * 1024)
		{
            //fatal_error("Not enough memory");
			throw ArcException("Not enough memory", bufsiz);
		}
    }
    return buf;
}

/* ------------------------------------------------------------------------ */
/* lh4, 5, 6, 7 */
void CStaticHuffman::encode_start_st1( /* void */ )
{
	// same as decode_start_st1() ?
	
    switch (dicbit) 
	{
    case LZHUFF4_DICBIT:
    case LZHUFF5_DICBIT: 
		pbit = 4; 
		np = LZHUFF5_DICBIT + 1; 
		break;
    case LZHUFF6_DICBIT: 
		pbit = 5; 
		np = LZHUFF6_DICBIT + 1; 
		break;
    case LZHUFF7_DICBIT: 
		pbit = 5; 
		np = LZHUFF7_DICBIT + 1; 
		break;
		
    default:
        //fatal_error("Cannot use %d bytes dictionary", 1 << dicbit);
		throw ArcException("Cannot use dictionary bytes", 1 << dicbit);
    }

	// why not use memset() here?
    int i = 0;
    for (i = 0; i < NC; i++)
	{
        c_freq[i] = 0;
	}
    for (i = 0; i < np; i++)
	{
        p_freq[i] = 0;
	}
    output_pos = output_mask = 0;
    m_BitIo.init_putbits();
    //init_code_cache();
    buf[0] = 0;
}

/* ------------------------------------------------------------------------ */
/* lh4, 5, 6, 7 */
void CStaticHuffman::encode_end_st1( /* void */ )
{
    if (!unpackable) 
	{
        send_block();
        putbits(CHAR_BIT - 1, 0);   /* flush remaining bits */
    }
}

/* ------------------------------------------------------------------------ */
/*                              decoding                                    */
/* ------------------------------------------------------------------------ */
void CStaticHuffman::read_pt_len(short nn, short nbit, short i_special)
{
    int           i, c, n;

    n = getbits(nbit);
    if (n == 0) 
	{
        c = getbits(nbit);
        for (i = 0; i < nn; i++)
		{
            pt_len[i] = 0;
		}
        for (i = 0; i < 256; i++)
		{
            pt_table[i] = c;
		}
    }
    else 
	{
        i = 0;
        while (i < n) 
		{
            c = peekbits(3);
            if (c != 7)
			{
                m_BitIo.fillbuf(3);
			}
            else 
			{
                unsigned short  mask = 1 << (16 - 4);
                while (mask & bitbuf) 
				{
                    mask >>= 1;
                    c++;
                }
                m_BitIo.fillbuf(c - 3);
            }

            pt_len[i++] = c;
            if (i == i_special) 
			{
                c = getbits(2);
                while (--c >= 0)
				{
                    pt_len[i++] = 0;
				}
            }
        }
        while (i < nn)
		{
            pt_len[i++] = 0;
		}
        make_table(nn, pt_len, 8, pt_table);
    }
}

/* ------------------------------------------------------------------------ */
void CStaticHuffman::read_c_len( /* void */ )
{
    short n = getbits(CBIT);
    if (n == 0) 
	{
        short c = getbits(CBIT);
		short i = 0;
        for (i = 0; i < NC; i++)
		{
            c_len[i] = 0;
		}
        for (i = 0; i < 4096; i++)
		{
            c_table[i] = c;
		}
    } 
	else 
	{
		short c = 0;
        short i = 0;
        while (i < n) 
		{
            c = pt_table[peekbits(8)];
            if (c >= NT) {
                unsigned short  mask = 1 << (16 - 9);
                do 
				{
                    if (bitbuf & mask)
					{
                        c = right[c];
					}
                    else
					{
                        c = left[c];
					}
                    mask >>= 1;
                } while (c >= NT);
            }
            m_BitIo.fillbuf(pt_len[c]);
            if (c <= 2) 
			{
                if (c == 0)
				{
                    c = 1;
				}
                else if (c == 1)
				{
                    c = getbits(4) + 3;
				}
                else
				{
                    c = getbits(CBIT) + 20;
				}
                while (--c >= 0)
				{
                    c_len[i++] = 0;
				}
            }
            else
			{
                c_len[i++] = c - 2;
			}
        }
        while (i < NC)
		{
            c_len[i++] = 0;
		}
        make_table(NC, c_len, 12, c_table);
    }
}

/* ------------------------------------------------------------------------ */
/* lh4, 5, 6, 7 */
unsigned short CStaticHuffman::decode_c_st1( /*void*/ )
{
    unsigned short  j, mask;

    if (blocksize == 0) 
	{
        blocksize = getbits(16);
        read_pt_len(NT, TBIT, 3);
        read_c_len();
        read_pt_len(np, pbit, -1);
    }
    blocksize--;
    j = c_table[peekbits(12)];
    if (j < NC)
	{
        m_BitIo.fillbuf(c_len[j]);
	}
    else 
	{
        m_BitIo.fillbuf(12);
        mask = 1 << (16 - 1);
        do 
		{
            if (bitbuf & mask)
			{
                j = right[j];
			}
            else
			{
                j = left[j];
			}
            mask >>= 1;
        } while (j >= NC);
        m_BitIo.fillbuf(c_len[j] - 12);
    }
    return j;
}

/* ------------------------------------------------------------------------ */
/* lh4, 5, 6, 7 */
unsigned short CStaticHuffman::decode_p_st1( /* void */ )
{
    unsigned short j = pt_table[peekbits(8)];
    if (j < np)
	{
        m_BitIo.fillbuf(pt_len[j]);
	}
    else 
	{
        m_BitIo.fillbuf(8);
        unsigned short mask = 1 << (16 - 1);
        do 
		{
            if (bitbuf & mask)
			{
                j = right[j];
			}
            else
			{
                j = left[j];
			}
            mask >>= 1;
        } while (j >= np);
		
        m_BitIo.fillbuf(pt_len[j] - 8);
    }
	
    if (j != 0)
	{
        j = (1 << (j - 1)) + getbits(j - 1);
	}
    return j;
}

/* ------------------------------------------------------------------------ */
/* lh4, 5, 6, 7 */
void CStaticHuffman::decode_start_st1( /* void */ )
{
	// same as encode_start_st1() ?
    switch (dicbit) 
	{
    case LZHUFF4_DICBIT:
    case LZHUFF5_DICBIT: 
		pbit = 4; 
		np = LZHUFF5_DICBIT + 1; 
		break;
    case LZHUFF6_DICBIT: 
		pbit = 5; 
		np = LZHUFF6_DICBIT + 1; 
		break;
    case LZHUFF7_DICBIT: 
		pbit = 5; 
		np = LZHUFF7_DICBIT + 1; 
		break;
    default:
        //fatal_error("Cannot use %d bytes dictionary", 1 << dicbit);
		throw ArcException("Cannot use dictionary bytes", 1 << dicbit);
    }

    m_BitIo.init_getbits();
    //init_code_cache();
    blocksize = 0;
}
