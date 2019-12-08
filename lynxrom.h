
#ifndef __LYNXROM_H__

#define MAXFILE 256

// Bytes should be 8-bits wide
typedef signed char SBYTE;
typedef unsigned char UBYTE;

// Words should be 16-bits wide
typedef signed short SWORD;
typedef unsigned short UWORD;

// Longs should be 32-bits wide
typedef int SLONG;
typedef unsigned int ULONG;

struct LNX_STRUCT
{
  UBYTE   magic[4];
  UWORD   page_size_bank0;
  UWORD   page_size_bank1;
  UWORD   version;
  UBYTE   cartname[32];
  UBYTE   manufname[16];
  UBYTE   rotation;
  UBYTE   spare[5];
};

struct FILE_PAR{
	char	*fname;

	unsigned char *memory;// Loaded data
	int		filesize;

	unsigned char *pointer;// written to rom
	unsigned char *data;// where in rom the entry starts (needed for dir), data afetr header
	int inromloadoffset;

	int		inromsize;// size stored in ROM
	int		loadadr;
	unsigned char flag;
	int		indirfilesize;// The file size stored in dir (TP packed!)
	
	int		dirpointer;// pointer to dir entry

	bool	blockalign;
	bool	bootpic;
	bool	entrymode;// 1 EPYX, 0 BLL
	int		copyof;// >=0 copy of another file
	int		newdiroffset;//
};


class lynxrom
{
private:
	short blocksize;
	short blockcount;
	int nMaxSize;

	bool verbose;
	bool skipheader;
	bool delimp;
	bool filler;
	bool fillrand;
	char fillerchar;
	bool useinternal;
	int  titleadr;
	int hackhead;

	bool writelyx;
	bool writelnx;

	bool lnxrot;
	char lnxmanu[65];
	char lnxname[65];
	
	int FILE_ANZ;
	struct FILE_PAR FILES[MAXFILE];
	int startdiradr;

	unsigned char *data;

	int oDirectoryPointer;// Pointer for  DIR entries
	int oCardTroyan;// heres the troyan horse (in DIR)
	int oEndDir;// End Dir = Data Start
	int nCartLen;
	
	unsigned long LoadFile(struct FILE_PAR *file);
	bool AnalyseFile(struct FILE_PAR *file);
	bool WriteFileToRom(struct FILE_PAR *file);
	void init_header(void);
	bool add_files(void);
	void ProcessFile(struct FILE_PAR &file);
	bool process_files(void);

	bool WriteDirCopyEntry(struct FILE_PAR *file,int off, bool mode);
	bool WriteDirEntry(struct FILE_PAR *file);
	
public:

	void init(void);
	bool init_rom(int bs,int bc);
	bool built(void);
	bool savelyx(char *fn);
	bool savelnx(char *fn);
	bool AddFile(char *fh,bool ins,bool align,bool mode,int offset);
	bool AddCopy(int nr,bool mode,int offset);
// bool lynxrom::AddFile(char *fname,bool bootpic=false,bool blockalign=false)

	unsigned long FileLength(char *fn);

	void copy_bll_header(void);

	void SetBlockSize(int s);
	inline void SetInternalLoader(void){ useinternal=true;};
	inline void SetDirStart(int n){ oDirectoryPointer =n;};
	inline void SetTroyan(void){ oCardTroyan = 0x0400;};
	inline void SetHackHeader(int n){hackhead=n;};

	inline void set_verbose(bool f){verbose=f;};
	inline void set_skipheader(bool f){skipheader=f;};
	inline void set_delimp(bool f){delimp=f;};
	inline void set_filler(bool f){filler=f;};
	inline void set_fillrand(bool f){fillrand=f;};
	inline void set_filler_zero(void){fillrand=false; fillerchar=0x00;};
	inline void set_filler_one(void){fillrand=false; fillerchar=0xFF;};
	inline void SetTitleAdr(int a){titleadr=a;};
	inline void set_write_lyx(bool f){writelyx=f;};
	inline void set_write_lnx(bool f){writelnx=f;};
	inline void set_lnxrot(bool f){lnxrot=f;};
	inline void set_lnxname(const char *c){strncpy(lnxname,c,64);};
	inline void set_lnxmanu(const char *c){strncpy(lnxmanu,c,64);};
};

#endif
