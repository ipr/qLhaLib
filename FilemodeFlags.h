/////////////////////////////////////
//
// misc. file-mode related flags
// and helper-structures to handle them 
// (used in LhA headers).
//

#ifndef FILEMODEFLAGS_H
#define FILEMODEFLAGS_H


typedef struct MsdosFlags
{
	// constructor
	MsdosFlags()
	{
		SetFromValue(0x20);
	}
	
	// some shitty MS-DOS-style flags (who cares about this?)
	bool bRo;  // bit1  read only
	bool bHid; // bit2  hidden
	bool bSys; // bit3  system
	bool bVol; // bit4  volume label
	bool bDir; // bit5  directory
	bool bArc; // bit6  archive bit (need to backup)
	
	void SetFromValue(unsigned char ucVal)
	{
		bRo = ((ucVal & 2) ? true : false);
		bHid = ((ucVal & 4) ? true : false);
		bSys = ((ucVal & 8) ? true : false);
		bVol = ((ucVal & 16) ? true : false);
		bDir = ((ucVal & 32) ? true : false);
		bArc = ((ucVal & 64) ? true : false);
	}
	
} MsdosFlags;


enum tUnixFlags
{
	UNIX_FILE_TYPEMASK     = 0170000,
	UNIX_FILE_REGULAR      = 0100000,
	UNIX_FILE_DIRECTORY    = 0040000,
	UNIX_FILE_SYMLINK      = 0120000,
	UNIX_SETUID            = 0004000,
	UNIX_SETGID            = 0002000,
	UNIX_STICKYBIT         = 0001000,
	UNIX_OWNER_READ_PERM   = 0000400,
	UNIX_OWNER_WRITE_PERM  = 0000200,
	UNIX_OWNER_EXEC_PERM   = 0000100,
	UNIX_GROUP_READ_PERM   = 0000040,
	UNIX_GROUP_WRITE_PERM  = 0000020,
	UNIX_GROUP_EXEC_PERM   = 0000010,
	UNIX_OTHER_READ_PERM   = 0000004,
	UNIX_OTHER_WRITE_PERM  = 0000002,
	UNIX_OTHER_EXEC_PERM   = 0000001,
	UNIX_RW_RW_RW          = 0000666
};

typedef struct UnixModeFlags
{
	// constructor
	UnixModeFlags()
	{
		// default: normal file with read+write for everyone
		unix_mode = UNIX_FILE_REGULAR | UNIX_RW_RW_RW;
	}
	
	//tUnixFlags m_enFlags;
	unsigned short  unix_mode;

	// is file just symbolic link instead of actual file?
	bool IsSymLink()
	{
		// TODO: UNIX_FILE_TYPEMASK ?
		if ((unix_mode & UNIX_FILE_SYMLINK) == UNIX_FILE_SYMLINK)
		{
			return true;
		}
		return false;
	}
	
	bool IsDirectory()
	{
		// TODO: UNIX_FILE_TYPEMASK ?
		if ((unix_mode & UNIX_FILE_DIRECTORY) == UNIX_FILE_DIRECTORY)
		{
			return true;
		}
		return false;
	}
	
} UnixModeFlags;


#endif // FILEMODEFLAGS_H