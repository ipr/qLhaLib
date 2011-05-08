///////////////////////////////////////////
//
// CLhDecoder : extract&decode file(s) from LhA-archive file to disk
//
// Ilkka Prusi 2011
//


#include "LhDecoder.h"

////////////////////// decoder base


void CLhDecoder::Decode(size_t &decode_count)
{
	unsigned int c = DecodeC();
	if (c < 256) 
	{
		m_dtext[m_loc++] = c;
		if (m_loc == m_dicsiz) 
		{
			m_uiCrc = m_crcio.calccrc(m_uiCrc, m_dtext, m_dicsiz);
			GetWriteBuf()->Append(m_dtext, m_dicsiz);
			m_loc = 0;
		}
		decode_count++;
	}
	else
	{
		int iMatchLen = c - m_adjust;
		unsigned int uiMatchOff = DecodeP() + 1; // may modify loc?
		unsigned int matchpos = (m_loc - uiMatchOff) & m_dicsiz_1;
		
		decode_count += iMatchLen;
		for (unsigned int i = 0; i < iMatchLen; i++) 
		{
			c = m_dtext[(matchpos + i) & m_dicsiz_1];
			m_dtext[m_loc++] = c;
			if (m_loc == m_dicsiz) 
			{
				m_uiCrc = m_crcio.calccrc(m_uiCrc, m_dtext, m_dicsiz);
				GetWriteBuf()->Append(m_dtext, m_dicsiz);
				m_loc = 0;
			}
		}
	}
}


////////////////////// decoders

// (note: -lh0-, -lhd- and -lz4- are "store only", no compression)

// -lh1-
// (dynamic huffman, shuffle)
void CLhDecodeLh1::DecodeStart(CReadBuffer *pReadBuf, CReadBuffer *pWriteBuf)
{
	m_loc = 0; // not updated, caller might want though

	// set to where this was inherited from
	m_BitIo.m_pReadBuf = pReadBuf;
	m_BitIo.m_pWriteBuf = pWriteBuf;
	
	// specific for this
	CShuffleHuffman::decode_start_fix();
}

unsigned short CLhDecodeLh1::DecodeC()
{
	return CDynamicHuffman::decode_c_dyn();
}

unsigned short CLhDecodeLh1::DecodeP()
{
	return CShuffleHuffman::decode_p_st0();
}

// -lh2-
// (dynamic huffman)
void CLhDecodeLh2::DecodeStart(CReadBuffer *pReadBuf, CReadBuffer *pWriteBuf)
{
	m_loc = 0; // not updated, caller might want though
	
	// set to where this was inherited from
	m_BitIo.m_pReadBuf = pReadBuf;
	m_BitIo.m_pWriteBuf = pWriteBuf;
	
	// specific for this
	CDynamicHuffman::decode_start_dyn();
}

unsigned short CLhDecodeLh2::DecodeC()
{
	return CDynamicHuffman::decode_c_dyn();
}

unsigned short CLhDecodeLh2::DecodeP()
{
	return CDynamicHuffman::decode_p_dyn();
}

// -lh3-
// (static huffman routine 0, shuffle)
void CLhDecodeLh3::DecodeStart(CReadBuffer *pReadBuf, CReadBuffer *pWriteBuf)
{
	m_loc = 0; // not updated, caller might want though
	
	// set to where this was inherited from
	m_BitIo.m_pReadBuf = pReadBuf;
	m_BitIo.m_pWriteBuf = pWriteBuf;
	
	CShuffleHuffman::decode_start_st0();
}

unsigned short CLhDecodeLh3::DecodeC()
{
	return CShuffleHuffman::decode_c_st0();
}

unsigned short CLhDecodeLh3::DecodeP()
{
	return CShuffleHuffman::decode_p_st0();
}

// -lh4- .. -lh7- (same for each)
// (static huffman routine 1)
void CLhDecodeLh7::DecodeStart(CReadBuffer *pReadBuf, CReadBuffer *pWriteBuf)
{
	m_loc = 0; // not updated, caller might want though

	// set to where this was inherited from
	m_BitIo.m_pReadBuf = pReadBuf;
	m_BitIo.m_pWriteBuf = pWriteBuf;
	
	CStaticHuffman::decode_start_st1();
}

unsigned short CLhDecodeLh7::DecodeC()
{
	return CStaticHuffman::decode_c_st1();
}

unsigned short CLhDecodeLh7::DecodeP()
{
	return CStaticHuffman::decode_p_st1();
}


// -lzs-

void CLhDecodeLzs::DecodeStart(CReadBuffer *pReadBuf, CReadBuffer *pWriteBuf)
{
	//decode_start_lzs
	m_loc = 0;

	// set to where this was inherited from
	m_BitIo.m_pReadBuf = pReadBuf;
	m_BitIo.m_pWriteBuf = pWriteBuf;
	
    m_BitIo.init_getbits();
}

unsigned short CLhDecodeLzs::DecodeC()
{
	//decode_c_lzs
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
	//decode_p_lzs
	// fucking globals again
    return (m_loc - m_matchpos - MAGIC0) & 0x7ff;
}

// -lz5-

void CLhDecodeLz5::DecodeStart(CReadBuffer *pReadBuf, CReadBuffer *pWriteBuf)
{
	//decode_start_lz5
	m_loc = 0;
    flagcnt = 0;

	// set to where this was inherited from
	m_BitIo.m_pReadBuf = pReadBuf;
	m_BitIo.m_pWriteBuf = pWriteBuf;
	
	/* no point in this since only encoding would use these??
    int i = 0;
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
	*/
}

unsigned short CLhDecodeLz5::DecodeC()
{
	//decode_c_lz5
    int c = 0;
    if (flagcnt == 0) 
	{
        flagcnt = 8;
		flag = m_BitIo.m_pReadBuf->GetNext();
    }
	
    flagcnt--;
	
	c = m_BitIo.m_pReadBuf->GetNext();
    if ((flag & 1) == 0) 
	{
        m_matchpos = c;

		c = m_BitIo.m_pReadBuf->GetNext();
		
        m_matchpos += (c & 0xf0) << 4;
        c &= 0x0f;
        c += 0x100;
    }
	
    flag >>= 1;
    return c;
}

unsigned short CLhDecodeLz5::DecodeP()
{
	//decode_p_lz5
	// fucking globals again
    return (m_loc - m_matchpos - MAGIC5) & 0xfff;
}

