#ifndef LHATYPEDEFS_H
#define LHATYPEDEFS_H

// ** user options **

// compression method/level
// user selectable option enum
// -> defuault to 7 ?
enum tUserCompressionMethod
{
	COMPRESS_5 = 5,
	COMPRESS_6 = 6,
	COMPRESS_7 = 7
};

// header level (compatibility)
// 0..2 are user selectable on file creation
// -> default to level 3?
enum tHeaderLevel
{
	HEADERLEVEL_0 = 0,
	HEADERLEVEL_1 = 1,
	HEADERLEVEL_2 = 2,
	HEADERLEVEL_3 = 3 // 
};

// for filename conversion
// -> use QTextCodec instead if necessary
enum tKanjiCodePage
{
	CODEPAGE_NONE  = 0,
	CODEPAGE_EUC   = 1,
	CODEPAGE_SJIS  = 2,
	CODEPAGE_UTF8  = 3,
	CODEPAGE_CAP   = 4    // Columbia AppleTalk Program 
};

// all compression methods
//
/* Added N.Watazaki ..V */
enum tCompressionMethod
{
	LZHUFF0_METHOD_NUM  = 0,
	LZHUFF1_METHOD_NUM  = 1,
	LZHUFF2_METHOD_NUM  = 2,
	LZHUFF3_METHOD_NUM  = 3,
	LZHUFF4_METHOD_NUM  = 4,
	LZHUFF5_METHOD_NUM  = 5,
	LZHUFF6_METHOD_NUM  = 6,
	LZHUFF7_METHOD_NUM  = 7,
	LARC_METHOD_NUM     = 8,
	LARC5_METHOD_NUM    = 9,
	LARC4_METHOD_NUM    = 10,
	LZHDIRS_METHOD_NUM  = 11
};
/* Added N.Watazaki ..^ */

// huffman encoding dictionary bits
//
enum tHuffBits
{
	LZHUFF0_DICBIT          = 0,      /* no compress */
	LZHUFF1_DICBIT          = 12,      /* 2^12 =  4KB sliding dictionary */
	LZHUFF2_DICBIT          = 13,      /* 2^13 =  8KB sliding dictionary */
	LZHUFF3_DICBIT          = 13,      /* 2^13 =  8KB sliding dictionary */
	LZHUFF4_DICBIT          = 12,      /* 2^12 =  4KB sliding dictionary */
	LZHUFF5_DICBIT          = 13,      /* 2^13 =  8KB sliding dictionary */
	LZHUFF6_DICBIT          = 15,      /* 2^15 = 32KB sliding dictionary */
	LZHUFF7_DICBIT          = 16,      /* 2^16 = 64KB sliding dictionary */
	LARC_DICBIT             = 11,      /* 2^11 =  2KB sliding dictionary */
	LARC5_DICBIT            = 12,      /* 2^12 =  4KB sliding dictionary */
	LARC4_DICBIT            = 0,      /* no compress */
							  
#ifdef SUPPORT_LH7
	 MAX_DICBIT         = LZHUFF7_DICBIT,      /* lh7 use 16bits */
#else
	 MAX_DICBIT         = LZHUFF6_DICBIT,      /* lh6 use 15bits */
#endif
						  
	 MAX_DICSIZ          = (1L << MAX_DICBIT)
};

#define LZHUFF0_METHOD          "-lh0-"
#define LZHUFF1_METHOD          "-lh1-"
#define LZHUFF2_METHOD          "-lh2-"
#define LZHUFF3_METHOD          "-lh3-"
#define LZHUFF4_METHOD          "-lh4-"
#define LZHUFF5_METHOD          "-lh5-"
#define LZHUFF6_METHOD          "-lh6-"
#define LZHUFF7_METHOD          "-lh7-"
#define LARC_METHOD             "-lzs-"
#define LARC5_METHOD            "-lz5-"
#define LARC4_METHOD            "-lz4-"
#define LZHDIRS_METHOD          "-lhd-"

#define EXTEND_GENERIC          0
#define EXTEND_UNIX             'U'
#define EXTEND_MSDOS            'M'
#define EXTEND_MACOS            'm'
#define EXTEND_OS9              '9'
#define EXTEND_OS2              '2'
#define EXTEND_OS68K            'K'
#define EXTEND_OS386            '3' /* OS-9000??? */
#define EXTEND_HUMAN            'H'
#define EXTEND_CPM              'C'
#define EXTEND_FLEX             'F'
#define EXTEND_RUNSER           'R'

/* this OS type is not official */
#define EXTEND_TOWNSOS          'T'
#define EXTEND_XOSK             'X' /* OS-9 for X68000 (?) */
#define EXTEND_JAVA             'J'

//#define GENERIC_ATTRIBUTE               0x20
//#define GENERIC_DIRECTORY_ATTRIBUTE     0x10

//#define CURRENT_UNIX_MINOR_VERSION      0x00

#define LHA_PATHSEP             0xff    /* path separator of the
                                           filename in lha header.
                                           it should compare with
                                           `unsigned char' or `int',
                                           that is not '\xff', but 0xff. */

#define OSK_RW_RW_RW            0000033
#define OSK_FILE_REGULAR        0000000
#define OSK_DIRECTORY_PERM      0000200
#define OSK_SHARED_PERM         0000100
#define OSK_OTHER_EXEC_PERM     0000040
#define OSK_OTHER_WRITE_PERM    0000020
#define OSK_OTHER_READ_PERM     0000010
#define OSK_OWNER_EXEC_PERM     0000004
#define OSK_OWNER_WRITE_PERM    0000002
#define OSK_OWNER_READ_PERM     0000001


#endif // LHATYPEDEFS_H
