///////////////////////////////////////////
//
// CLhDecoder : extract&decode file(s) from LhA-archive file to disk
//
// Ilkka Prusi 2011
//


#include "LhDecoder.h"


////////////////////// decoders

// (note: -lh0-, -lhd- and -lz4- are "store only", no compression)

// -lh1-
// (dynamic huffman, shuffle)
void CLhDecodeLh1::DecodeStart()
{
	// specific for this
	CShuffleHuffman::decode_start_fix();
}

unsigned short CLhDecodeLh1::DecodeC()
{
	return CDynamicHuffman::decode_c_dyn_huf();
}

unsigned short CLhDecodeLh1::DecodeP()
{
	return CShuffleHuffman::decode_p_st0_huf();
}

// -lh2-
// (dynamic huffman)
void CLhDecodeLh2::DecodeStart()
{
	// specific for this
	CDynamicHuffman::decode_start_dyn();
}

unsigned short CLhDecodeLh2::DecodeC()
{
	return CDynamicHuffman::decode_c_dyn_huf();
}

unsigned short CLhDecodeLh2::DecodeP()
{
	return CDynamicHuffman::decode_p_dyn_huf();
}

// -lh3-
// (static huffman routine 0, shuffle)
void CLhDecodeLh3::DecodeStart()
{
	CShuffleHuffman::decode_start_st0();
}

unsigned short CLhDecodeLh3::DecodeC()
{
	return CShuffleHuffman::decode_c_st0_huf();
}

unsigned short CLhDecodeLh3::DecodeP()
{
	return CShuffleHuffman::decode_p_st0_huf();
}

// -lh4- .. -lh7- (same for each)
// (static huffman routine 1)
void CLhDecodeLh7::DecodeStart()
{
	CStaticHuffman::decode_start_st1();
}

unsigned short CLhDecodeLh7::DecodeC()
{
	return CStaticHuffman::decode_c_st1_huf();
}

unsigned short CLhDecodeLh7::DecodeP()
{
	return CStaticHuffman::decode_p_st1_huf();
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

