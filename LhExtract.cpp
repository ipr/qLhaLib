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
	//CLhDecoder *pDecoder = nullptr;
	
	// (note: -lh0-, -lhd- and -lz4- are "store only", no compression)
	
	// -lh1-
	m_mapDecoders.insert(tDecorders::value_type(LZHUFF1_METHOD_NUM, new CLhDecodeLh1()));
	
	// -lh2-
	m_mapDecoders.insert(tDecorders::value_type(LZHUFF2_METHOD_NUM, new CLhDecodeLh2()));
	
	// -lh3-
	m_mapDecoders.insert(tDecorders::value_type(LZHUFF3_METHOD_NUM, new CLhDecodeLh3()));

	// -lh4- .. -lh7- -> same decoding (different instance)
	m_mapDecoders.insert(tDecorders::value_type(LZHUFF4_METHOD_NUM, new CLhDecodeLh7()));
	m_mapDecoders.insert(tDecorders::value_type(LZHUFF5_METHOD_NUM, new CLhDecodeLh7()));
	m_mapDecoders.insert(tDecorders::value_type(LZHUFF6_METHOD_NUM, new CLhDecodeLh7()));
	m_mapDecoders.insert(tDecorders::value_type(LZHUFF7_METHOD_NUM, new CLhDecodeLh7()));
	
	// -lzs-
	m_mapDecoders.insert(tDecorders::value_type(LARC_METHOD_NUM, new CLhDecodeLzs()));
	
	// -lz5-
	m_mapDecoders.insert(tDecorders::value_type(LARC5_METHOD_NUM, new CLhDecodeLz5()));
}

CLhDecoder *CLhExtract::GetDecoder(const tCompressionMethod enMethod)
{
	// TODO: just create new instance every time decoding is needed?
	// (would simplify resetting stuff..)
	
	auto it = m_mapDecoders.find(enMethod);
	if (it != m_mapDecoders.end())
	{
		return it->second;
	}
	return nullptr;
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
unsigned int CLhExtract::ExtractDecode(CAnsiFile &ArchiveFile, LzHeader *pHeader, CAnsiFile &OutFile)
{
	CLhDecoder *pDecoder = GetDecoder(m_Compression);
	if (pDecoder == nullptr)
	{
		throw ArcException("Unknown/unsupported compression", m_Compression);
	}
	
	// prepare enough in buffers, zero them also
	m_ReadBuf.PrepareBuffer(pHeader->packed_size, false);
	m_WriteBuf.PrepareBuffer(pHeader->original_size, false);
	
	if (ArchiveFile.Read(m_ReadBuf.GetBegin(), pHeader->packed_size) == false)
	{
		throw IOException("Failed reading input");
	}

	pDecoder->InitClear();
	pDecoder->InitDictionary(m_Compression, m_HuffBits);
	pDecoder->InitBitIo(pHeader->packed_size, pHeader->original_size, &m_ReadBuf, &m_WriteBuf);
	
	pDecoder->DecodeStart();
	
	size_t decode_count = 0;
    while (decode_count < pHeader->original_size) 
	{
		pDecoder->Decode(decode_count);
	}
	
	pDecoder->DecodeFinish();

	// write to output upto what is collected in write-buffer
	if (OutFile.Write(m_WriteBuf.GetBegin(), m_WriteBuf.GetCurrentPos()) == false)
	{
		throw IOException("Failed writing output");
	}

	return pDecoder->GetCrc();
}

// extract "store only":
// -lh0-, -lhd- and -lz4- 
// -> no compression -> "as-is"
//
unsigned int CLhExtract::ExtractNoCompression(CAnsiFile &ArchiveFile, LzHeader *pHeader, CAnsiFile &OutFile)
{
	// no compression, just copy to output
	
	// check we have enough buffer for reading in chunks
	m_ReadBuf.PrepareBuffer(4096, false);
	
	unsigned int uiFileCrc = 0;
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
		uiFileCrc = m_crcio.calccrc(uiFileCrc, m_ReadBuf.GetBegin(), read_size);
		
		// write output
		if (OutFile.Write(m_ReadBuf.GetBegin(), read_size) == false)
		{
			throw IOException("Failed writing output");
		}
		nToWrite -= read_size;
	}
	
	// file done
	return uiFileCrc;
}


/////// public methods


// decode data from archive and write to prepared output file,
// use given metadata as help..
//
void CLhExtract::ExtractFile(CAnsiFile &ArchiveFile, LzHeader *pHeader, CAnsiFile &OutFile)
{
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

	unsigned int uiFileCrc = 0;
	
	// huffman dictionary bits
	m_HuffBits = GetDictionaryBits(m_Compression);
	if ((int)m_HuffBits == 0)
	{
		// no compression, just copy to output
		// (-lh0-, -lhd- and -lz4-)
		
		uiFileCrc = ExtractNoCompression(ArchiveFile, pHeader, OutFile);
		
		// file done
	}
	else
	{
		// LZ decoding: need decoder for the method
		//
		uiFileCrc = ExtractDecode(ArchiveFile, pHeader, OutFile);

		// file done
	}
	
	// flush to disk (whatever we may have..)
	if (OutFile.Flush() == false)
	{
		throw IOException("Failed flushing output");
	}
	
	// verify CRC: exception if not match (keep decoded anyway..)
	if (pHeader->has_crc == true && pHeader->crc != uiFileCrc)
	{
		throw ArcException("CRC error on extract", pHeader->filename.toStdString());
	}
}

