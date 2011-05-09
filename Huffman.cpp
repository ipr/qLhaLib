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
short CHuffmanTree::make_tree(int nchar, unsigned short *freq, unsigned char *bitlen, unsigned short *code) const
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


//// CShuffleHuffman


/* ------------------------------------------------------------------------ */
/* LHa for UNIX                                                             */
/*              shuf.c -- extract static Huffman coding                     */
/*                                                                          */
/*      Modified                Nobutaka Watazaki                           */
/*                                                                          */
/*  Ver. 1.14   Source All chagned              1995.01.14  N.Watazaki      */
/* ------------------------------------------------------------------------ */

// static member array initialization,
// only used by ready_made()
const int CShuffleHuffman::fixed[2][16] = {
	{3, 0x01, 0x04, 0x0c, 0x18, 0x30, 0},   /* old compatible */
	{2, 0x01, 0x01, 0x03, 0x06, 0x0D, 0x1F, 0x4E, 0}    /* 8K buf */
};

/* ------------------------------------------------------------------------ */
void CShuffleHuffman::ready_made(int method)
{
    const int *tbl = fixed[method];
    int j = *tbl++;
    unsigned int weight = 1 << (16 - j);
    unsigned int code = 0;
	
    for (int i = 0; i < m_np; i++) 
	{
        while (*tbl == i) 
		{
            j++;
            tbl++;
            weight >>= 1;
        }
		// pt_len and pt_code in dynamic huffman??
        pt_len[i] = j;
        pt_code[i] = code;
        code += weight;
    }
}

/* ------------------------------------------------------------------------ */
/* lh3 */
void CShuffleHuffman::decode_start_st0( /*void*/ )
{
    n_max = 286;
    maxmatch = MAXMATCH;
    m_BitIo.init_getbits();
    //init_code_cache(); // EUC<->SJIS
    m_np = 1 << (LZHUFF3_DICBIT - 6);
}

/* ------------------------------------------------------------------------ */
void CShuffleHuffman::encode_p_st0(unsigned short  j)
{
    unsigned short i = j >> 6;
    m_BitIo.putcode(pt_len[i], pt_code[i]);
    m_BitIo.putbits(6, j & 0x3f);
}

/* ------------------------------------------------------------------------ */
/* lh1 */
void CShuffleHuffman::encode_start_fix( /*void*/ )
{
    n_max = 314;
    maxmatch = 60;
    m_np = 1 << (12 - 6);
    m_BitIo.init_putbits();
    //init_code_cache(); // EUC<->SJIS
    start_c_dyn();
    ready_made(0);
}

/* ------------------------------------------------------------------------ */
void CShuffleHuffman::read_tree_c( /*void*/ )
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
		
		//const int nBitCount = CBIT;
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
void CShuffleHuffman::read_tree_p(/*void*/)
{
	/* read tree from file */

    int i = 0;
    while (i < SHUF_NP) 
	{
        pt_len[i] = m_BitIo.getbits(SHUF_LENFIELD);
		
		//const int nBitCount = LZHUFF3_DICBIT - 6;
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
/* lh1 */
void CShuffleHuffman::decode_start_fix(/*void*/)
{
    n_max = 314;
    maxmatch = 60;
    m_BitIo.init_getbits();
    //init_code_cache(); // EUC<->SJIS
    m_np = 1 << (LZHUFF1_DICBIT - 6);
    start_c_dyn();
    ready_made(0);
    make_table(m_np, pt_len, 8, pt_table);
}

/* ------------------------------------------------------------------------ */
/* lh3 */
unsigned short CShuffleHuffman::decode_c_st0(/*void*/)
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
            ready_made(1);
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
unsigned short CShuffleHuffman::decode_p_st0(/*void*/)
{
    int j = pt_table[peekbits(8)];
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
    return (j << 6) + getbits(6);
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
	SetDictBit(enBit);
	
    n_max = 286;
    maxmatch = MAXMATCH;
    m_BitIo.init_getbits();
    //init_code_cache(); // EUC<->SJIS
	
    start_c_dyn(); // shared with -lh1-
	
    freq[ROOT_P] = 1;
    child[ROOT_P] = ~(N_CHAR);
    s_node[N_CHAR] = ROOT_P;
    edge[block[ROOT_P] = stock[avail++]] = ROOT_P;
    most_p = ROOT_P;
    total_p = 0;
    m_nn = m_dicbit;
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
        for (int k = start; f < freq[k]; k++);
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
void CDynamicHuffman::encode_c_dyn(unsigned int c)
{
    int d = c - n1;
    if (d >= 0) 
	{
        c = n1;
    }
	
    unsigned int bits = 0;
    int cnt = 0;
    int p = s_node[c];
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
        m_BitIo.putcode(cnt, bits >> 16);
    } 
	else 
	{
        m_BitIo.putcode(16, bits >> 16);
        m_BitIo.putbits(cnt - 16, bits);
    }
	
    if (d >= 0)
	{
        m_BitIo.putbits(8, d);
	}
    update_c(c);
}

/* ------------------------------------------------------------------------ */
/* lh1, 2 */
unsigned short CDynamicHuffman::decode_c_dyn( /* void */ )
{
    int c = child[ROOT_C];
    short buf = bitbuf;
    short cnt = 0;
	
    do 
	{
        c = child[c - (buf < 0)];
        buf <<= 1;
        if (++cnt == 16) 
		{
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
        c += m_BitIo.getbits(8);
	}
    return c;
}

/* ------------------------------------------------------------------------ */
/* lh2 */
unsigned short CDynamicHuffman::decode_p_dyn( /* void */ )
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
    m_BitIo.putcode(7, 0);
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
/*                              Encording                                   */
/* ------------------------------------------------------------------------ */
void CStaticHuffman::count_t_freq(/*void*/)
{
    short           count;

	clear_t_freq();
	
    short n = NC;
    while (n > 0 && c_len[n - 1] == 0)
	{
        n--;
	}
	
    short i = 0;
    while (i < n) 
	{
        short k = c_len[i++];
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
    while (n > 0 && pt_len[n - 1] == 0)
	{
        n--;
	}
    m_BitIo.putbits(nbit, n);
	
    short i = 0;
    while (i < n) 
	{
        short k = pt_len[i++];
        if (k <= 6)
		{
            m_BitIo.putbits(3, k);
		}
        else
		{
            /* k=7 -> 1110  k=8 -> 11110  k=9 -> 111110 ... */
            m_BitIo.putbits(k - 3, USHRT_MAX << 1);
		}
        if (i == i_special) 
		{
            while (i < 6 && pt_len[i] == 0)
			{
                i++;
			}
            m_BitIo.putbits(2, i - 3);
        }
    }
}

/* ------------------------------------------------------------------------ */
void CStaticHuffman::write_c_len(/*void*/)
{
    short           count;

    short n = NC;
    while (n > 0 && c_len[n - 1] == 0)
	{
        n--;
	}
	
    m_BitIo.putbits(CBIT, n);
	
    short i = 0;
    while (i < n) 
	{
        short k = c_len[i++];
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
                    m_BitIo.putcode(pt_len[0], pt_code[0]);
				}
            }
            else if (count <= 18) 
			{
                m_BitIo.putcode(pt_len[1], pt_code[1]);
                m_BitIo.putbits(4, count - 3);
            }
            else if (count == 19) 
			{
                m_BitIo.putcode(pt_len[0], pt_code[0]);
                m_BitIo.putcode(pt_len[1], pt_code[1]);
                m_BitIo.putbits(4, 15);
            }
            else 
			{
                m_BitIo.putcode(pt_len[2], pt_code[2]);
                m_BitIo.putbits(CBIT, count - 20);
            }
        }
        else
		{
            m_BitIo.putcode(pt_len[k + 2], pt_code[k + 2]);
		}
    }
}

/* ------------------------------------------------------------------------ */
void CStaticHuffman::encode_c(short c)
{
    m_BitIo.putcode(c_len[c], c_code[c]);
}

/* ------------------------------------------------------------------------ */
void CStaticHuffman::encode_p(unsigned short  p)
{
    unsigned short c = 0;
    unsigned short q = p;
	
    while (q) 
	{
        q >>= 1;
        c++;
    }
	
    m_BitIo.putcode(pt_len[c], pt_code[c]);
	
    if (c > 1)
	{
        m_BitIo.putbits(c - 1, p);
	}
}

/* ------------------------------------------------------------------------ */
void CStaticHuffman::send_block( /* void */ )
{
    unsigned char   flags;
    unsigned short  i, k, pos;

    unsigned short root = make_tree(NC, c_freq, c_len, c_code);
    unsigned short size = c_freq[root];
	
    m_BitIo.putbits(16, size);
	
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
            m_BitIo.putbits(TBIT, 0);
            m_BitIo.putbits(TBIT, root);
        }
        write_c_len();
    } 
	else 
	{
        m_BitIo.putbits(TBIT, 0);
        m_BitIo.putbits(TBIT, 0);
        m_BitIo.putbits(CBIT, 0);
        m_BitIo.putbits(CBIT, root);
    }
	
    root = make_tree(m_np, p_freq, pt_len, pt_code);
    if (root >= m_np) 
	{
        write_pt_len(m_np, m_pbit, -1);
    }
    else 
	{
        m_BitIo.putbits(m_pbit, 0);
        m_BitIo.putbits(m_pbit, root);
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
        if (m_BitIo.unpackable)
		{
            return;
		}
    }
	
	// reduce duplication
	clear_c_p_freq();
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
            if (m_BitIo.unpackable)
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
void CStaticHuffman::encode_start_st1(const tHuffBits enBit)
{
	// same as decode_start_st1()
	SetDictBit(enBit);
	if (SetByDictbit(enBit) == false)
	{
		throw ArcException("Cannot use dictionary bytes", m_dicbit);
	}

	// reduce duplication
	clear_c_p_freq();
	
    output_pos = output_mask = 0;
    m_BitIo.init_putbits();
    //init_code_cache(); // EUC<->SJIS
    buf[0] = 0;
}

/* ------------------------------------------------------------------------ */
/* lh4, 5, 6, 7 */
void CStaticHuffman::encode_end_st1( /* void */ )
{
    if (!m_BitIo.unpackable) 
	{
        send_block();
        m_BitIo.putbits(CHAR_BIT - 1, 0);   /* flush remaining bits */
    }
}

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
void CStaticHuffman::read_c_len( /* void */ )
{
    short n = m_BitIo.getbits(CBIT);
    if (n == 0) 
	{
        short c = m_BitIo.getbits(CBIT);
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
			unsigned short bit = m_BitIo.peekbits(8);
            c = pt_table[bit];
			
            if (c >= NT) 
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
    if (m_blocksize == 0) 
	{
        m_blocksize = m_BitIo.getbits(16);
        read_pt_len(NT, TBIT, 3);
        read_c_len();
        read_pt_len(m_np, m_pbit, -1);
    }
    m_blocksize--;
	
	unsigned short bit = m_BitIo.peekbits(12);
    unsigned short j = c_table[bit];
    if (j < NC)
	{
        m_BitIo.fillbuf(c_len[j]);
	}
    else 
	{
        m_BitIo.fillbuf(12);
		
		decode_st1_mask_bitbuf(j, NC);
		
        m_BitIo.fillbuf(c_len[j] - 12);
    }
    return j;
}

/* ------------------------------------------------------------------------ */
/* lh4, 5, 6, 7 */
unsigned short CStaticHuffman::decode_p_st1( /* void */ )
{
	unsigned short bit = m_BitIo.peekbits(8);
    unsigned short j = pt_table[bit];
    if (j < m_np)
	{
        m_BitIo.fillbuf(pt_len[j]);
	}
    else 
	{
        m_BitIo.fillbuf(8);
		
		decode_st1_mask_bitbuf(j, m_np);
		
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
	// same as encode_start_st1()
	SetDictBit(enBit);
	if (SetByDictbit(enBit) == false)
	{
		throw ArcException("Cannot use dictionary bytes", m_dicbit);
	}
	
    m_BitIo.init_getbits();
    //init_code_cache(); // EUC<->SJIS
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

// used by decode_start_st1() and encode_start_st1(),
// reduce duplication of stuff
bool CStaticHuffman::SetByDictbit(const tHuffBits enBit)
{
	bool bRet = false;
    switch (enBit) 
	{
    case LZHUFF4_DICBIT:
    case LZHUFF5_DICBIT: 
		m_pbit = 4; 
		m_np = LZHUFF5_DICBIT + 1; 
		bRet = true; // supported
		break;
		
    case LZHUFF6_DICBIT: 
		m_pbit = 5; 
		m_np = LZHUFF6_DICBIT + 1; 
		bRet = true; // supported
		break;
		
    case LZHUFF7_DICBIT: 
		m_pbit = 5; 
		m_np = LZHUFF7_DICBIT + 1; 
		bRet = true; // supported
		break;
		
    default:
		break;
    }
	return bRet;
}

// some reduction of duplication,
// also replace zeroing-loop by simple memset()
// (likely to be faster anyway)
//
void CStaticHuffman::clear_c_p_freq()
{
	// use memset(), likely to be faster
	// and those arrays are not overlapping either..
	memset(c_freq, 0, sizeof(unsigned short)*NC);
	memset(p_freq, 0, sizeof(unsigned short)*m_np);

	// note: old had bug in resetting part of arrays?
	// (actual sizes (2 * NC - 1) and (2 * NP - 1))
	
	/*
	// why not use memset() here?
	// it's not overlapping or anything..
	int i = 0;
	for (i = 0; i < NC; i++)
	{
		c_freq[i] = 0;
	}
	for (i = 0; i < m_np; i++)
	{
		p_freq[i] = 0;
	}
	*/
}
	
void CStaticHuffman::clear_t_freq()
{
	// use memset(), likely to be faster
	memset(t_freq, 0, sizeof(unsigned short)*NT);

	// note: old had bug in resetting part of array?
	// (actual size (2 * NT - 1))	
	
	/*
    for (short l = 0; l < NT; l++)
	{
        t_freq[l] = 0;
	}
	*/
}

