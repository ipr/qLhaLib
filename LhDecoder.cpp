///////////////////////////////////////////
//
// CLhDecoder : extract&decode file(s) from LhA-archive file to disk
//
// Ilkka Prusi 2011
//


#include "LhDecoder.h"


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

////////////////////// decoders

// (note: -lh0-, -lhd- and -lz4- are "store only", no compression)

// -lh1-
// partially same as -lh2-
// (dynamic huffman routine)

void CLhDecodeLh1::DecodeStart()
{
}

unsigned short CLhDecodeLh1::DecodeC()
{
}

unsigned short CLhDecodeLh1::DecodeP()
{
}

// -lh2-
// partially same as -lh1-
// (dynamic huffman routine)

void CLhDecodeLh2::DecodeStart()
{
}

unsigned short CLhDecodeLh2::DecodeC()
{
}

unsigned short CLhDecodeLh2::DecodeP()
{
}

// -lh3-
// (static huffman routine 0)

void CLhDecodeLh3::DecodeStart()
{
}

unsigned short CLhDecodeLh3::DecodeC()
{
}

unsigned short CLhDecodeLh3::DecodeP()
{
}

// -lh4- .. -lh7-
// -> same decoding (check virtual methods)
// (static huffman routine 1)

void CLhDecodeLh7::DecodeStart()
{
}

unsigned short CLhDecodeLh7::DecodeC()
{
}

unsigned short CLhDecodeLh7::DecodeP()
{
}


// -lzs-

void CLhDecodeLzs::DecodeStart()
{
	m_loc = 0;
    m_BitIo.init_getbits();
}

unsigned short CLhDecodeLzs::DecodeC()
{
    if (m_BitIo.getbits(1)) 
	{
        return m_BitIo.getbits(8);
    }
    else 
	{
        m_matchpos = m_BitIo.getbits(11);
        return m_BitIo.getbits(4) + 0x100;
    }
}

unsigned short CLhDecodeLzs::DecodeP()
{
	// fucking globals again
    return (m_loc - m_matchpos - MAGIC0) & 0x7ff;
}

// -lz5-

void CLhDecodeLz5::DecodeStart()
{
    int i = 0;
	m_loc = 0;

    flagcnt = 0;
    for (i = 0; i < 256; i++)
	{
        memset(&text[i * 13 + 18], i, 13);
	}
    for (i = 0; i < 256; i++)
	{
        text[256 * 13 + 18 + i] = i;
	}
    for (i = 0; i < 256; i++)
	{
        text[256 * 13 + 256 + 18 + i] = 255 - i;
	}
    memset(&text[256 * 13 + 512 + 18], 0, 128);
    memset(&text[256 * 13 + 512 + 128 + 18], ' ', 128 - 18);
}

unsigned short CLhDecodeLz5::DecodeC()
{
    int c = 0;

    if (flagcnt == 0) 
	{
        flagcnt = 8;
		flag = m_pReadBuf->GetNext();
    }
	
    flagcnt--;
	
	c = m_pReadBuf->GetNext();
    if ((flag & 1) == 0) 
	{
        m_matchpos = c;

		c = m_pReadBuf->GetNext();
		
        m_matchpos += (c & 0xf0) << 4;
        c &= 0x0f;
        c += 0x100;
    }
	
    flag >>= 1;
    return c;
}

unsigned short CLhDecodeLz5::DecodeP()
{
	// fucking globals again
    return (m_loc - m_matchpos - MAGIC5) & 0xfff;
}

