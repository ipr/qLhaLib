#include "Huffman.h"

#include <stdlib.h>


/////////// CHuffmanTree


/* ------------------------------------------------------------------------ */
/* LHa for UNIX                                                             */
/*              maketree.c -- make Huffman tree                             */
/*                                                                          */
/*      Modified                Nobutaka Watazaki                           */
/*                                                                          */
/*  Ver. 1.14   Source All chagned              1995.01.14  N.Watazaki      */
/* ------------------------------------------------------------------------ */

void CHuffmanTree::make_code(int nchar, 
			   unsigned char  *bitlen, 
			   unsigned short *code,       /* table */
			   unsigned short *leaf_num) const
{
    unsigned short  weight[17]; /* 0x10000ul >> bitlen */
    unsigned short  start[17];  /* start code */
    unsigned short total = 0;
	
    for (int i = 1; i <= 16; i++) 
	{
        start[i] = total;
        weight[i] = 1 << (16 - i);
        total += weight[i] * leaf_num[i];
    }
	
    for (int c = 0; c < nchar; c++) 
	{
        int l = bitlen[c];
        code[c] = start[l];
        start[l] += weight[l];
    }
}

void CHuffmanTree::count_leaf(int node, /* call with node = root */
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

void CHuffmanTree::make_len(int nchar, 
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
void CHuffmanTree::downheap(int i, short *heap, size_t heapsize, unsigned short *freq) const
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
short CHuffmanTree::make_tree(int nchar, unsigned short *freq, unsigned char *bitlen, unsigned short *code)
{
    short i, j, avail, root;
    unsigned short *sort;

    short heap[NC_LEN + 1];       /* NC >= nchar */
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


/* ------------------------------------------------------------------------ */
/* LHa for UNIX                                                             */
/*              maketbl.c -- makes decoding table                           */
/*                                                                          */
/*      Modified                Nobutaka Watazaki                           */
/*                                                                          */
/*  Ver. 1.14   Source All chagned              1995.01.14  N.Watazaki      */
/* ------------------------------------------------------------------------ */

void CHuffmanTree::make_table(
    short           nchar,
    unsigned char   bitlen[],
    short           tablebits,
    unsigned short  table[])
{
	// note: loops below are 1..<=16 -> need 17 cells
    unsigned short  count[17];  /* count of bitlen */
    unsigned short  weight[17]; /* 0x10000ul >> bitlen */
    unsigned short  start[17];  /* first code of bitlen */
    unsigned int    iCount;

    int avail = nchar;

    /* initialize */
	::memset(count, 0, sizeof(unsigned short)*17);
    for (iCount = 1; iCount <= 16; iCount++) 
	{
        //count[iCount] = 0;
        weight[iCount] = 1 << (16 - iCount);
    }

    /* count */
    for (iCount = 0; iCount < nchar; iCount++)
	{
        count[bitlen[iCount]]++;
	}

    /* calculate first code */
    unsigned short total = 0;
    for (iCount = 1; iCount <= 16; iCount++) 
	{
        start[iCount] = total;
        total += weight[iCount] * count[iCount];
    }
    if ((total & 0xffff) != 0)
	{
        //error("make_table(): Bad table (5)");
		throw ArcException("make_table(): Bad table (5)", total);
	}

    /* shift data for make table. */
    int m = 16 - tablebits;
    for (iCount = 1; iCount <= tablebits; iCount++) 
	{
        start[iCount] >>= m;
        weight[iCount] >>= m;
    }

    /* initialize */
    int start_m = start[tablebits + 1] >> m;
    if (start_m != 0)
	{
		int kEnd = 1 << tablebits;
        for (int i = start_m; i < kEnd; i++)
		{
            table[i] = 0;
		}
	}

    /* create table and tree */
    for (int j = 0; j < nchar; j++) 
	{
		// unconditional set -> make scoped
        int k = bitlen[j];
        if (k == 0)
		{
            continue;
		}
		
        unsigned int l = start[k] + weight[k];
        if (k <= tablebits) 
		{
            /* code in table */
            for (unsigned int i = start[k]; i < l; i++)
			{
                table[i] = j;
			}
        }
        else 
		{
            /* code not in table */
			unsigned int i = start[k];
			
            unsigned short *pTbl = &table[i >> m];
            i <<= tablebits;
			
            int n = k - tablebits;
			
            /* make tree (n length) */
			make_table_tree(n, j, i, pTbl, avail);
			
            /* make tree (n length) */
			/*
            while (--n >= 0) 
			{
                if (*p == 0) 
				{
                    right[avail] = left[avail] = 0;
                    *p = avail++;
                }
                if (i & 0x8000)
				{
                    p = &right[*p];
				}
                else
				{
                    p = &left[*p];
				}
                i <<= 1;
            }
            *p = j;
			*/
        }
        start[k] = l;
    }
}

// split from above: this is called in loop from make_table(),
// only part in make_table() that uses left[] and right[] arrays
//
void CHuffmanTree::make_table_tree(int nLen, const int j, unsigned int &i, unsigned short *pTbl, int &avail)
{
	/* make tree (n length) */
    while (--nLen >= 0) 
	{
        if ((*pTbl) == 0) 
		{
            right[avail] = left[avail] = 0;
            (*pTbl) = avail++;
        }
        if (i & 0x8000)
		{
            pTbl = &right[(*pTbl)];
		}
        else
		{
            pTbl = &left[(*pTbl)];
		}
        i <<= 1;
    }
    (*pTbl) = j;
}


//// CShuffleHuffman


/* ------------------------------------------------------------------------ */
/* LHa for UNIX                                                             */
/*              shuf.c -- extract static Huffman coding                     */
/*                                                                          */
/*      Modified                Nobutaka Watazaki                           */
/*                                                                          */
/*  Ver. 1.14   Source All chagned              1995.01.14  N.Watazaki      */
/* ------------------------------------------------------------------------ */


// these lookup-tables only used by -lh1- and -lh2-
const int CShuffleHuffman::fixed_method_lh1[16] = {3, 0x01, 0x04, 0x0c, 0x18, 0x30, 0};   // old compatible 
const int CShuffleHuffman::fixed_method_lh3[16] = {2, 0x01, 0x01, 0x03, 0x06, 0x0D, 0x1F, 0x4E, 0};    // 8K buf 


void CShuffleHuffman::fixed_method_pt_len(const int *tbl)
{
    //const int *tbl = fixed[method];
    int j = (*tbl);
	tbl++;
	
    unsigned int weight = 1 << (16 - j);
    unsigned int code = 0;
	
    for (int i = 0; i < m_np; i++) 
	{
        while ((*tbl) == i) 
		{
            j++;
            tbl++;
            weight >>= 1;
        }
        pt_len[i] = j;
        code += weight;
    }
}

/* ------------------------------------------------------------------------ */
/* lh1 */
void CShuffleHuffman::decode_start_fix()
{
	// call to base-class
	CHuffman::init_decode_start(314, 60);
	m_BitIo.init_getbits();
	
    m_np = 1 << (LZHUFF1_DICBIT - 6);
	
    start_c_dyn();

    fixed_method_pt_len(fixed_method_lh1);
    make_table(m_np, pt_len, 8, pt_table);
}

/* ------------------------------------------------------------------------ */
/* lh3 */
void CShuffleHuffman::decode_start_st0()
{
	// call to base-class
	CHuffman::init_decode_start(286, MAXMATCH);
	m_BitIo.init_getbits();
	
    m_np = 1 << (LZHUFF3_DICBIT - 6);
}

/* ------------------------------------------------------------------------ */
void CShuffleHuffman::read_tree_c()
{
	/* read tree from file */
    int i = 0;
    while (i < SHUF_N1) 
	{
		unsigned short bit = m_BitIo.getbits(1);
        if (bit)
		{
            c_len[i] = m_BitIo.getbits(SHUF_LENFIELD) + 1;
		}
        else
		{
            c_len[i] = 0;
		}
		
        if (++i == 3 && c_len[0] == 1 && c_len[1] == 1 && c_len[2] == 1) 
		{
            int c = m_BitIo.getbits(CBIT);
            for (i = 0; i < SHUF_N1; i++)
			{
                c_len[i] = 0;
			}
            for (i = 0; i < 4096; i++)
			{
                c_table[i] = c;
			}
            return;
        }
    }
    make_table(SHUF_N1, c_len, 12, c_table);
}

/* ------------------------------------------------------------------------ */
void CShuffleHuffman::read_tree_p()
{
	/* read tree from file */

    int i = 0;
    while (i < SHUF_NP) 
	{
        pt_len[i] = m_BitIo.getbits(SHUF_LENFIELD);
		
        if (++i == 3 && pt_len[0] == 1 && pt_len[1] == 1 && pt_len[2] == 1) 
		{
            int c = m_BitIo.getbits(LZHUFF3_DICBIT - 6);
            for (i = 0; i < SHUF_NP; i++)
			{
                pt_len[i] = 0;
			}
            for (i = 0; i < 256; i++)
			{
                pt_table[i] = c;
			}
            return;
        }
    }
}

/* ------------------------------------------------------------------------ */
/* lh3 */
unsigned short CShuffleHuffman::decode_c_st0()
{
    if (m_blocksize == 0)    /* read block head */
	{
        m_blocksize = m_BitIo.getbits(SHUF_BUFBITS);   /* read block blocksize */
		
        read_tree_c();
		
		unsigned short bit = m_BitIo.getbits(1);
        if (bit) 
		{
            read_tree_p();
        }
        else 
		{
		    fixed_method_pt_len(fixed_method_lh3);
        }
        make_table(SHUF_NP, pt_len, 8, pt_table);
    }
    m_blocksize--;
	
	unsigned short bit = m_BitIo.peekbits(12);
    int j = c_table[bit];
    if (j < SHUF_N1)
	{
        m_BitIo.fillbuf(c_len[j]);
	}
    else 
	{
        m_BitIo.fillbuf(12);
        int i = m_BitIo.bitbuf;
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
        } while (j >= SHUF_N1);
        m_BitIo.fillbuf(c_len[j] - 12);
    }
    if (j == SHUF_N1 - 1)
	{
        j += m_BitIo.getbits(SHUF_EXTRABITS);
	}
    return j;
}

/* ------------------------------------------------------------------------ */
/* lh1, 3 */
unsigned short CShuffleHuffman::decode_p_st0()
{
	unsigned short bit = m_BitIo.peekbits(8);
    int j = pt_table[bit];
    if (j < m_np) 
	{
        m_BitIo.fillbuf(pt_len[j]);
    }
    else 
	{
        m_BitIo.fillbuf(8);
        int i = m_BitIo.bitbuf;
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
        } while (j >= m_np);
        m_BitIo.fillbuf(pt_len[j] - 8);
    }
    return (j << 6) + m_BitIo.getbits(6);
}

//// CDynamicHuffman

/* ------------------------------------------------------------------------ */
/* LHa for UNIX                                                             */
/*              dhuf.c -- Dynamic Hufffman routine                          */
/*                                                                          */
/*      Modified                H.Yoshizaki                                 */
/*                                                                          */
/*  Ver. 1.14   Source All chagned              1995.01.14  N.Watazaki      */
/* ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ */

void CDynamicHuffman::start_c_dyn()
{
    int             i, j;

    n1 = (n_max >= 256 + m_maxmatch - THRESHOLD + 1) ? 512 : n_max - 1;
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
        int f = freq[j] = freq[i] + freq[i - 1];
        child[j] = i;
        parent[i] = parent[i - 1] = j;
        if (f == freq[j + 1]) 
		{
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
/* lh2 */
void CDynamicHuffman::decode_start_dyn(const tHuffBits enBit)
{
	// call to base-class
	CHuffman::init_decode_start(286, MAXMATCH);
	m_BitIo.init_getbits();
	
    start_c_dyn(); // shared with -lh1-
	
    freq[ROOT_P] = 1;
    child[ROOT_P] = ~(N_CHAR);
    s_node[N_CHAR] = ROOT_P;
    edge[block[ROOT_P] = stock[avail++]] = ROOT_P;
    most_p = ROOT_P;
    total_p = 0;

	m_nn = (1 << ((int)enBit));
    nextcount = 64;
}

/* ------------------------------------------------------------------------ */
void CDynamicHuffman::reconst(int start, int end)
{
    int i, j;
	int b;

    for (i = j = start; i < end; i++) 
	{
		int k = child[i];
        if (k < 0) 
		{
            freq[j] = (freq[i] + 1) / 2;
            child[j] = k;
            j++;
        }
		b = block[i];
        if (edge[b] == i) 
		{
            stock[--avail] = b;
        }
    }
    j--;
    i = end - 1;
	
    int l = end - 2;
	
    while (i >= start) 
	{
        while (i >= l) 
		{
            freq[i] = freq[j];
            child[i] = child[j];
            i--, j--;
        }
		
        unsigned int f = freq[l] + freq[l + 1];
		
		int k = 0;
        for (k = start; f < freq[k]; k++); // note: empty loop?
		
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
	
    unsigned int f = 0;
    for (i = start; i < end; i++) 
	{
		j = child[i];
        if (j < 0)
		{
            s_node[~j] = i;
		}
        else
		{
            parent[j] = parent[j - 1] = i;
		}
		
		unsigned int g = freq[i];
        if (g == f) 
		{
            block[i] = b;
        }
        else 
		{
			b = block[i] = stock[avail++];
            edge[b] = i;
            f = g;
        }
    }
}

/* ------------------------------------------------------------------------ */
int CDynamicHuffman::swap_inc(int p)
{
    int b = block[p];
	int q = edge[b]; 
    if (q != p) /* swap for leader */
	{
        int r = child[p];
        int s = child[q];
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

// was a goto-segment..
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
    int r = most_p + 1;
    int q = r + 1;
	
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
/* lh1, 2 */
unsigned short CDynamicHuffman::decode_c_dyn()
{
    int c = child[ROOT_C];
    short buf = m_BitIo.bitbuf;
    short cnt = 0;
	
    do 
	{
        c = child[c - (buf < 0)];
        buf <<= 1;
        if (++cnt == 16) 
		{
            m_BitIo.fillbuf(16);
            buf = m_BitIo.bitbuf;
            cnt = 0;
        }
    } while (c > 0);
	
    m_BitIo.fillbuf(cnt);
    c = ~c;
    update_c(c);
	
    if (c == n1)
	{
        c += m_BitIo.getbits(8);
	}
    return c;
}

/* ------------------------------------------------------------------------ */
/* lh2 */
unsigned short CDynamicHuffman::decode_p_dyn(size_t &decode_count)
{
    while (decode_count > nextcount) 
	{
        make_new_node(nextcount / 64);
        if ((nextcount += 64) >= m_nn)
		{
            nextcount = 0xffffffff;
		}
    }
	
    int c = child[ROOT_P];
    short buf = m_BitIo.bitbuf;
    short cnt = 0;
	
    while (c > 0) 
	{
        c = child[c - (buf < 0)];
        buf <<= 1;
        if (++cnt == 16) 
		{
            m_BitIo.fillbuf(16);
            buf = m_BitIo.bitbuf;
            cnt = 0;
        }
    }
	
    m_BitIo.fillbuf(cnt);
    c = (~c) - N_CHAR;
    update_p(c);

    return (c << 6) + m_BitIo.getbits(6);
}

//// CStaticHuffman


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
/*                              decoding                                    */
/* ------------------------------------------------------------------------ */
void CStaticHuffman::read_pt_len(short nn, short nbit, short i_special)
{
    int n = m_BitIo.getbits(nbit);
    if (n == 0) 
	{
		int i = 0;
        int c = m_BitIo.getbits(nbit);
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
        int i = 0;
        while (i < n) 
		{
            int c = m_BitIo.peekbits(3);
            if (c != 7)
			{
                m_BitIo.fillbuf(3);
			}
            else 
			{
                unsigned short  mask = 1 << (16 - 4);
                while (mask & m_BitIo.bitbuf) 
				{
                    mask >>= 1;
                    c++;
                }
                m_BitIo.fillbuf(c - 3);
            }

            pt_len[i++] = c;
            if (i == i_special) 
			{
                c = m_BitIo.getbits(2);
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
void CStaticHuffman::read_c_len()
{
    short n = m_BitIo.getbits(CBIT);
    if (n == 0) 
	{
        short c = m_BitIo.getbits(CBIT);
		short i = 0;
        for (i = 0; i < NC_LEN; i++)
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
			unsigned short bit = m_BitIo.peekbits(8);
            c = pt_table[bit];
			
            if (c >= NT_LEN) 
			{
                unsigned short  mask = 1 << (16 - 9);
                do 
				{
                    if (m_BitIo.bitbuf & mask)
					{
                        c = right[c];
					}
                    else
					{
                        c = left[c];
					}
                    mask >>= 1;
                } while (c >= NT_LEN);
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
                    c = m_BitIo.getbits(4) + 3;
				}
                else
				{
                    c = m_BitIo.getbits(CBIT) + 20;
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
		
        while (i < NC_LEN)
		{
            c_len[i++] = 0;
		}
        make_table(NC_LEN, c_len, 12, c_table);
    }
}

/* ------------------------------------------------------------------------ */
/* lh4, 5, 6, 7 */
unsigned short CStaticHuffman::decode_c_st1()
{
    if (m_blocksize == 0) 
	{
        m_blocksize = m_BitIo.getbits(16);
        read_pt_len(NT_LEN, 5, 3);
        read_c_len();
        read_pt_len(m_np_dict, m_dict_bit, -1);
    }
    m_blocksize--;
	
	unsigned short bit = m_BitIo.peekbits(12);
    unsigned short j = c_table[bit];
    if (j < NC_LEN)
	{
        m_BitIo.fillbuf(c_len[j]);
	}
    else 
	{
        m_BitIo.fillbuf(12);
		
		decode_st1_mask_bitbuf(j, NC_LEN);
		
        m_BitIo.fillbuf(c_len[j] - 12);
    }
    return j;
}

/* ------------------------------------------------------------------------ */
/* lh4, 5, 6, 7 */
unsigned short CStaticHuffman::decode_p_st1()
{
	unsigned short bit = m_BitIo.peekbits(8);
    unsigned short j = pt_table[bit];
    if (j < m_np_dict)
	{
        m_BitIo.fillbuf(pt_len[j]);
	}
    else 
	{
        m_BitIo.fillbuf(8);
		
		decode_st1_mask_bitbuf(j, m_np_dict);
		
        m_BitIo.fillbuf(pt_len[j] - 8);
    }
	
    if (j != 0)
	{
        j = (1 << (j - 1)) + m_BitIo.getbits(j - 1);
	}
    return j;
}

/* ------------------------------------------------------------------------ */
/* lh4, 5, 6, 7 */
void CStaticHuffman::decode_start_st1(const tHuffBits enBit)
{
	bool bRet = false;
    switch (enBit) 
	{
    case LZHUFF4_DICBIT:
    case LZHUFF5_DICBIT: 
		m_dict_bit = 4; 
		m_np_dict = LZHUFF5_DICBIT + 1; 
		bRet = true; // supported
		break;
		
    case LZHUFF6_DICBIT: 
		m_dict_bit = 5; 
		m_np_dict = LZHUFF6_DICBIT + 1; 
		bRet = true; // supported
		break;
		
    case LZHUFF7_DICBIT: 
		m_dict_bit = 5; 
		m_np_dict = LZHUFF7_DICBIT + 1; 
		bRet = true; // supported
		break;
		
	default: // silence some compilers
		break;
    }

	// unknown/unsupport here
	if (bRet == false)
	{
		throw ArcException("Cannot use dictionary bytes", (int)enBit);
	}
	
    m_BitIo.init_getbits();
    m_blocksize = 0;
}

// reduce duplication
void CStaticHuffman::decode_st1_mask_bitbuf(unsigned short &j, const int nCount)
{
	unsigned short mask = 1 << (16 - 1);
	do 
	{
		if (m_BitIo.bitbuf & mask)
		{
			j = right[j];
		}
		else
		{
			j = left[j];
		}
		mask >>= 1;
	} while (j >= nCount);
}

