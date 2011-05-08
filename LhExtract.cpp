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
	
	// (note: -lh0-, -lhd- and -lz4- are "store only", no compression)
	
	// -lh1-
	pDecoder = new CLhDecodeLh1();
	pDecoder->CreateDecoder();
	m_mapDecoders.insert(LZHUFF1_METHOD_NUM, pDecoder);
	
	// -lh2-
	pDecoder = new CLhDecodeLh2();
	pDecoder->CreateDecoder();
	m_mapDecoders.insert(LZHUFF2_METHOD_NUM, pDecoder);
	
	// -lh3-
	pDecoder = new CLhDecodeLh3();
	pDecoder->CreateDecoder();
	m_mapDecoders.insert(LZHUFF3_METHOD_NUM, pDecoder);

	// -lh4- .. -lh7- -> same decoding
	pDecoder = new CLhDecodeLh7();
	pDecoder->CreateDecoder();
	m_mapDecoders.insert(LZHUFF4_METHOD_NUM, pDecoder);
	m_mapDecoders.insert(LZHUFF5_METHOD_NUM, pDecoder);
	m_mapDecoders.insert(LZHUFF6_METHOD_NUM, pDecoder);
	m_mapDecoders.insert(LZHUFF7_METHOD_NUM, pDecoder);
	
	// -lzs-
	pDecoder = new CLhDecodeLzs();
	pDecoder->CreateDecoder();
	m_mapDecoders.insert(LARC_METHOD_NUM, pDecoder);
	
	// -lz5-
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
        return LZHUFF0_DICBIT; // -> "store only", not compressed
		
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
        return LARC4_DICBIT; // -> "store only", not compressed
		
	case LZHDIRS_METHOD_NUM:    /* -lhd- */
        return LARC4_DICBIT; // -> "store only", not compressed
		
    default:
		break;
    }

	//warning("unknown method %d", method);
	return LZHUFF5_DICBIT; /* for backward compatibility */
}

// extract with decoding (compressed):
// -lh1- .. -lh7-, -lzs-, -lz5-
// -> different decoders and variations needed
//
void CLhExtract::ExtractDecode(CAnsiFile &ArchiveFile, LzHeader *pHeader, CAnsiFile &OutFile)
{
	CLhDecoder *pDecoder = GetDecoder(m_Compression);
	
	// we do this above..
    //decode_set = decode_define[interface->method - 1];
	
	pDecoder->SetBuffers(&m_ReadBuf, &m_WriteBuf);
	
	// prepare enough in buffers, zero them also
	m_ReadBuf.PrepareBuffer(pHeader->packed_size, false);
	m_WriteBuf.PrepareBuffer(pHeader->original_size, false);
	
	if (ArchiveFile.Read(m_ReadBuf.GetBegin(), pHeader->packed_size) == false)
	{
		throw IOException("Failed reading input");
	}

	size_t nToRead = pHeader->packed_size;
	size_t nToWrite = pHeader->original_size;

	
    unsigned int adjust = 0; // inline in decode() in slide.c
    //unsigned int dicsiz1 = 0; // inline in decode() in slide.c..
	//unsigned long dicsiz = 0; // static in slide.c
    unsigned long dicsiz = (1L << (int)m_HuffBits); // yes, it's enum now..
	
	// out-buffer (dictionary-lookup result)? local only?
	// -> use our write-buffer for this?
	// -> or move to decoder?
	//
	// use from stack for automatic cleanup on leaving scope
	// (destructor called, exception or normal return)
	CReadBuffer BufDictText(dicsiz);
	unsigned char *dtext = BufDictText.GetBegin();
	
	//memset(dtext, 0, dicsiz); // for broken archive only? why?
	memset(dtext, ' ', dicsiz);
	
    //decode_set.decode_start(); // initalize&set tables?
	pDecoder->DecodeStart();
	
    unsigned int dicsiz1 = dicsiz - 1; // why separate?
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


    while (decode_count < origsize) 
	{
        //c = decode_set.decode_c();
		c = pDecoder->DecodeC();
        if (c < 256) 
		{
			unsigned long loc = pDecoder->GetLoc();
            dtext[loc++] = c;
            if (loc == dicsiz) 
			{
				m_uiCrc = m_crcio.calccrc(m_uiCrc, dtext, dicsiz);
				m_WriteBuf.Append(dtext, dicsiz);
                loc = 0;
            }
			pDecoder->SetLoc(loc);
            decode_count++;
		}
		else
		{
            //struct matchdata match; // given to some methods but in encoding only
            //unsigned int matchpos; // also needed in decoder.. (-lzs- and -lz5- anyway)
            //match.len = c - adjust;
            //match.off = decode_set.decode_p() + 1;

			// TODO: reduce these variables, see about reducing repeated stuff
			//
			int iMatchLen = c - adjust;
            unsigned int uiMatchOff = pDecoder->DecodeP() + 1; // may modify loc?
			unsigned long loc = pDecoder->GetLoc();
            unsigned int matchpos = (loc - uiMatchOff) & dicsiz1;
			
            decode_count += iMatchLen;
            for (i = 0; i < iMatchLen; i++) 
			{
                c = dtext[(matchpos + i) & dicsiz1];
                dtext[loc++] = c;
                if (loc == dicsiz) 
				{
					m_uiCrc = m_crcio.calccrc(m_uiCrc, dtext, dicsiz);
					m_WriteBuf.Append(dtext, dicsiz);
                    loc = 0;
                }
			}
			
			pDecoder->SetLoc(loc);
		}
	}
	
    if (pDecoder->GetLoc() != 0) 
	{
		unsigned long loc = pDecoder->GetLoc();
		m_uiCrc = m_crcio.calccrc(m_uiCrc, dtext, loc);
		m_WriteBuf.Append(dtext, loc);
    }

	// write to output upto what is collected in write-buffer
	if (OutFile.Write(m_WriteBuf.GetBegin(), m_WriteBuf.GetCurrentPos()) == false)
	{
		throw IOException("Failed writing output");
	}
	
	
    /* usually read size is interface->packed */
    //interface->read_size = interface->packed - compsize;

	//nToRead = pHeader->packed_size - compsize;
    //return m_uiCrc;
	
	// below is best-guess what should be done,
	// see decoding for how large changes may be needed..
	//
	/*
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
	*/
}

// extract "store only":
// -lh0-, -lhd- and -lz4- 
// -> no compression -> "as-is"
//
void CLhExtract::ExtractNoCompression(CAnsiFile &ArchiveFile, LzHeader *pHeader, CAnsiFile &OutFile)
{
	// no compression, just copy to output
	
	// check we have enough buffer for reading in chunks
	m_ReadBuf.PrepareBuffer(4096, false);
	
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
		
		// update crc
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
	if (m_Compression == LZ_UNKNOWN)
	{
		// unknown/unsupported method
		return;
	}
	
	/*
	if (m_Compression == LZHDIRS_METHOD_NUM)
	{
		// just make directories and no actual files ?
		return;
	}
	*/

	// huffman dictionary bits
	m_HuffBits = GetDictionaryBits(m_Compression);
	if ((int)m_HuffBits == 0)
	{
		// no compression, just copy to output
		// (-lh0-, -lhd- and -lz4-)
		
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
	if (OutFile.Flush() == false)
	{
		throw IOException("Failed flushing output");
	}
	
	// verify CRC
	if (pHeader->has_crc == true && pHeader->crc != m_uiCrc)
	{
		//throw ArcException("CRC error on extract", pHeader->filename.toStdString());
	}
}

