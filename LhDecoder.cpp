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
			subbitbuf = m_pReadBuffer[m_nReadPos++];
            //subbitbuf = (unsigned char) getc(infile);
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
void BitIo::putcode(unsigned char   n, unsigned short  x)
{
    while (n >= bitcount) 
	{
        n -= bitcount;
        subbitbuf += x >> (USHRT_BIT - bitcount);
        x <<= bitcount;
        if (compsize < origsize) 
		{
			m_pWriteBuffer[m_nWritePos++] = subbitbuf;
			/*
            if (fwrite(&subbitbuf, 1, 1, outfile) == 0) 
			{
                fatal_error("Write error in bitio.c(putcode)");
            }
			*/
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

