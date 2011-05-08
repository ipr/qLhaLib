///////////////////////////////////////////
//
// CLhExtract : extract&decode file(s) from LhA-archive file to disk
//
// Ilkka Prusi 2011
//


#include "LhExtract.h"

/////// private methods

void CLhExtract::CreateDecoders()
{
	CLhDecoder *pDecoder = nullptr;
	
	pDecoder = new CLhDecodeLh1();
	pDecoder->CreateDecoder();
	m_mapDecoders.insert(LZHUFF1_METHOD_NUM, pDecoder);
	
	pDecoder = new CLhDecodeLh2();
	pDecoder->CreateDecoder();
	m_mapDecoders.insert(LZHUFF2_METHOD_NUM, pDecoder);
	
	pDecoder = new CLhDecodeLh3();
	pDecoder->CreateDecoder();
	m_mapDecoders.insert(LZHUFF3_METHOD_NUM, pDecoder);
	
	pDecoder = new CLhDecodeLh4();
	pDecoder->CreateDecoder();
	m_mapDecoders.insert(LZHUFF4_METHOD_NUM, pDecoder);
	
	pDecoder = new CLhDecodeLh5();
	pDecoder->CreateDecoder();
	m_mapDecoders.insert(LZHUFF5_METHOD_NUM, pDecoder);
	
	pDecoder = new CLhDecodeLh6();
	pDecoder->CreateDecoder();
	m_mapDecoders.insert(LZHUFF6_METHOD_NUM, pDecoder);
	
	pDecoder = new CLhDecodeLh7();
	pDecoder->CreateDecoder();
	m_mapDecoders.insert(LZHUFF7_METHOD_NUM, pDecoder);
	
	pDecoder = new CLhDecodeLzs();
	pDecoder->CreateDecoder();
	m_mapDecoders.insert(LARC_METHOD_NUM, pDecoder);
	
	pDecoder = new CLhDecodeLz5();
	pDecoder->CreateDecoder();
	m_mapDecoders.insert(LARC5_METHOD_NUM, pDecoder);
}

CLhDecoder *CLhExtract::GetDecoder(const tCompressionMethod enMethod)
{
	return m_mapDecoders.value(enMethod, nullptr);
}

/////// protected methods

tHuffBits CLhExtract::GetDictionaryBits(const tCompressionMethod enMethod) const
{
    switch (enMethod) 
	{
    case LZHUFF0_METHOD_NUM:    /* -lh0- */
        return LZHUFF0_DICBIT;
    case LZHUFF1_METHOD_NUM:    /* -lh1- */
        return LZHUFF1_DICBIT;
    case LZHUFF2_METHOD_NUM:    /* -lh2- */
        return LZHUFF2_DICBIT;
    case LZHUFF3_METHOD_NUM:    /* -lh2- */
        return LZHUFF3_DICBIT;
    case LZHUFF4_METHOD_NUM:    /* -lh4- */
        return LZHUFF4_DICBIT;
    case LZHUFF5_METHOD_NUM:    /* -lh5- */
        return LZHUFF5_DICBIT;
    case LZHUFF6_METHOD_NUM:    /* -lh6- */
        return LZHUFF6_DICBIT;
    case LZHUFF7_METHOD_NUM:    /* -lh7- */
        return LZHUFF7_DICBIT;
    case LARC_METHOD_NUM:       /* -lzs- */
        return LARC_DICBIT;
    case LARC5_METHOD_NUM:      /* -lz5- */
        return LARC5_DICBIT;
    case LARC4_METHOD_NUM:      /* -lz4- */
        return LARC4_DICBIT;
		
    default:
		break;
    }

	//warning("unknown method %d", method);
	return LZHUFF5_DICBIT; /* for backward compatibility */
}

void CLhExtract::ExtractDecode(CAnsiFile &ArchiveFile, LzHeader *pHeader, CAnsiFile &OutFile)
{
	CLhDecoder *pDecoder = GetDecoder(pHeader->GetMethod());
	
	// we do this above..
    //decode_set = decode_define[interface->method - 1];

	size_t nToRead = pHeader->packed_size;
	size_t nToWrite = pHeader->original_size;
	
    unsigned int adjust = 0; // inline in decode() in slide.c
    //unsigned int dicsiz1 = 0; // inline in decode() in slide.c..
	unsigned long dicsiz = 0; // static in slide.c
    dicsiz = (1L << (int)m_HuffBits); // yes, it's enum now..
	
	// out-buffer? local only?
    unsigned char *dtext = (unsigned char *)malloc(dicsiz);
	//memset(dtext, 0, dicsiz); // for broken archive only? why?
	memset(dtext, ' ', dicsiz);
	
    //decode_set.decode_start(); // initalize&set tables?
	pDecoder->DecodeStart();
	
    //dicsiz1 = dicsiz - 1; // why separate?
    adjust = 256 - THRESHOLD;
    if (m_Compression == LARC_METHOD_NUM)
	{
        adjust = 256 - 2;
	}

	// some more globals to locals..
	// let's guess where these are used later..
	size_t decode_count = 0;
    //unsigned long loc = 0;
	// -> pass loc to decoder ?
	// -> moved to decoder

	// function pointers in struct:
	//{decode_c, decode_p, decode_start}
    /* lh1 */
    //{decode_c_dyn, decode_p_st0, decode_start_fix},
    /* lh2 */
    //{decode_c_dyn, decode_p_dyn, decode_start_dyn},
    /* lh3 */
    //{decode_c_st0, decode_p_st0, decode_start_st0},
    /* lh4 */
    //{decode_c_st1, decode_p_st1, decode_start_st1},
    /* lh5 */
    //{decode_c_st1, decode_p_st1, decode_start_st1},
    /* lh6 */
    //{decode_c_st1, decode_p_st1, decode_start_st1},
    /* lh7 */
    //{decode_c_st1, decode_p_st1, decode_start_st1},
    /* lzs */
    //{decode_c_lzs, decode_p_lzs, decode_start_lzs},
    /* lz5 */
    //{decode_c_lz5, decode_p_lz5, decode_start_lz5}

    while (decode_count < origsize) 
	{
        c = decode_set.decode_c();
        if (c < 256) 
		{
            dtext[loc++] = c;
            if (loc == dicsiz) 
			{
				m_uiCrc = m_crcio.calccrc(m_uiCrc, dtext, dicsiz);
				memcpy(m_pWriteBuffer + m_nWritePos, dtext, dicsiz);
				m_nWritePos += dicsiz;
                loc = 0;
            }
            decode_count++;
		}
		else
		{
            struct matchdata match;
            unsigned int matchpos;

            match.len = c - adjust;
            match.off = decode_set.decode_p() + 1;
            matchpos = (loc - match.off) & dicsiz1;
            decode_count += match.len;
            for (i = 0; i < match.len; i++) 
			{
                c = dtext[(matchpos + i) & dicsiz1];
                dtext[loc++] = c;
                if (loc == dicsiz) 
				{
					m_uiCrc = m_crcio.calccrc(m_uiCrc, dtext, dicsiz);
					memcpy(m_pWriteBuffer + m_nWritePos, dtext, dicsiz);
					m_nWritePos += dicsiz;
                    loc = 0;
                }
			}
		}
	}
    if (loc != 0) 
	{
		m_uiCrc = m_crcio.calccrc(m_uiCrc, dtext, loc);
		memcpy(m_pWriteBuffer + m_nWritePos, dtext, loc);
		m_nWritePos += loc;
    }

    free(dtext);
	
    /* usually read size is interface->packed */
    //interface->read_size = interface->packed - compsize;

	//nToRead = pHeader->packed_size - compsize;
    //return m_uiCrc;
	
	// below is best-guess what should be done,
	// see decoding for how large changes may be needed..
	//
	while (nToWrite > 0)
	{
		size_t read_size = nToWrite;
		if (read_size > 4096)
		{
			read_size = 4096;
		}
		
		// read chunk
		if (ArchiveFile.Read(m_ReadBuf.GetBegin(), read_size) == false)
		{
			throw IOException("Failed reading data");
		}
		
		unsigned char *pReadBuf = m_ReadBuf.GetBegin();
		while (read_size > 0)
		{
			// decode
			//pDecoder->Decode(pReadBuf, read_size);
			
			// update crc (could make static instance..)
			//m_uiCrc = m_crcio.calccrc(m_uiCrc, pDecoder->GetDecoded(), pDecoder->GetDecodedSize());
			
			// write to output
			if (OutFile.Write(pDecoder->GetDecoded(), pDecoder->GetDecodedSize()) == false)
			{
				throw IOException("Failed writing output");
			}
			
			nToWrite -= pDecoder->GetDecodedSize();
			read_size -= pDecoder->GetReadPackedSize();
			pReadBuf = pReadBuf + pDecoder->GetReadPackedSize();
		}
	}
}


void CLhExtract::ExtractNoCompression(CAnsiFile &ArchiveFile, LzHeader *pHeader, CAnsiFile &OutFile)
{
	// no compression, just copy to output
	
	size_t nToWrite = pHeader->original_size;
	while (nToWrite > 0)
	{
		size_t read_size = nToWrite;
		if (read_size > 4096)
		{
			read_size = 4096;
		}
		
		if (ArchiveFile.Read(m_ReadBuf.GetBegin(), read_size) == false)
		{
			throw IOException("Failed reading data");
		}
		
		// update crc (could make static instance..)
		m_uiCrc = m_crcio.calccrc(m_uiCrc, m_ReadBuf.GetBegin(), read_size);
		
		// write output
		if (OutFile.Write(m_ReadBuf.GetBegin(), read_size) == false)
		{
			throw IOException("Failed writing output");
		}
		nToWrite -= read_size;
	}
	
	// file done
}


/////// public methods


// decode data from archive and write to prepared output file,
// use given metadata as help..
//
void CLhExtract::ExtractFile(CAnsiFile &ArchiveFile, LzHeader *pHeader, CAnsiFile &OutFile)
{
	m_uiCrc = 0;
	
	// determine decoding method
	m_Compression = pHeader->GetMethod();
	if (m_Compression == LZHDIRS_METHOD_NUM)
	{
		// just make directories and no actual files ?
		return;
	}

	// huffman dictionary bits
	m_HuffBits = GetDictionaryBits(m_Compression);
	if ((int)m_HuffBits == 0)
	{
		// no compression, just copy to output
		
		ExtractNoCompression(ArchiveFile, pHeader, OutFile);
		
		// file done
	}
	else
	{
		// LZ decoding: need decoder for the method
		//
		ExtractDecode(ArchiveFile, pHeader, OutFile);

		// file done
	}
	
	// flush to disk
	OutFile.Flush();
	
	// verify CRC
	if (pHeader->has_crc == true && pHeader->crc != m_uiCrc)
	{
		//throw ArcException("CRC error on extract", pHeader->filename.toStdString());
	}
}

