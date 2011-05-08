///////////////////////////////////////////
//
// CLhExtract : extract&decode file(s) from LhA-archive file to disk
//
// Ilkka Prusi 2011
//

#ifndef LHEXTRACT_H
#define LHEXTRACT_H

#include <QObject>
#include <QString>
#include <QMap>

#include "AnsiFile.h"
#include "LhaTypeDefs.h"
#include "LhHeader.h"
#include "crcio.h"

#include "LhDecoder.h"


class CLhExtract : public QObject
{
private:
	QMap<tCompressionMethod, CLhDecoder*> m_mapDecoders;
	void CreateDecoders();
	
	inline CLhDecoder *GetDecoder(const tCompressionMethod enMethod);
	
protected:
	CCrcIo m_crcio;
	
	CReadBuffer m_ReadBuf; // packed data from archive
	CReadBuffer m_WriteBuf; // unpacked data for writing
	
	// kept and updated when extracting file
	unsigned int m_uiCrc;
	tCompressionMethod m_Compression;
	tHuffBits m_HuffBits;
	
	tHuffBits GetDictionaryBits(const tCompressionMethod enMethod) const;

	void ExtractDecode(CAnsiFile &ArchiveFile, LzHeader *pHeader, CAnsiFile &OutFile);
	
	void ExtractNoCompression(CAnsiFile &ArchiveFile, LzHeader *pHeader, CAnsiFile &OutFile);

	
public:
    CLhExtract(QObject *parent = 0)
		: QObject(parent)
		, m_mapDecoders()
		, m_ReadBuf(4096) // emulated old buffering style
		, m_WriteBuf(2* 4096)
		, m_crcio()
		, m_uiCrc(0)
	{
		CreateDecoders();
	};
    ~CLhExtract()
	{
		auto it = m_mapDecoders.begin();
		auto itEnd = m_mapDecoders.end();
		while (it != itEnd)
		{
			CLhDecoder *pDec = it.value();
			delete pDec;
			++it;
		}
		m_mapDecoders.clear();
	};
	
	
	void ExtractFile(CAnsiFile &ArchiveFile, LzHeader *pHeader, CAnsiFile &OutFile);
};

#endif // LHEXTRACT_H
