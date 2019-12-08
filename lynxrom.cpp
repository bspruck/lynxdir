#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>

#include "lynxrom.h"

#define CARDHEADLEN 0x0408
#define HEADERLEN 10

// unsigned char CardHead[CARDHEADLEN];		// 1032
#include "loader27.h"
#include "dinsert.h"

#include "hack_l1.h"
#include "hack_l2_512.h"
#include "hack_l2_1024.h"
#include "hack_l2_2048.h"

struct LNX_STRUCT lnxhead;


/*************************************************************
*** FileLength                                             ***
*** Returns the filelength                                 ***
*************************************************************/
unsigned long lynxrom::FileLength(char *fn)
{
	unsigned long len;
	FILE *fh;
	
	fh = fopen(fn,"rb");
	if(fh==0) return(0);
	
	fseek(fh,0L,SEEK_END);
	len = ftell(fh);
	fclose(fh);
	return (len);
}

/*************************************************************
*** LoadFile                                               ***
*************************************************************/
unsigned long lynxrom::LoadFile(struct FILE_PAR *file)
{
	FILE *fh;
	
	if( file->fname==0) return(0);	
	fh = fopen(file->fname,"rb");
	if(fh==0) return(0);
	
	file->memory=new unsigned char[file->filesize];
	file->memory[file->filesize-1]=0xFF;
	if(file->filesize!=fread(file->memory,1,file->filesize,fh)){
		printf("%s: File Size changed??\n",file->fname);	
	}
	fclose(fh);
	
	return (file->filesize);
}


void lynxrom::init(void)
{
	verbose=false;
	skipheader=true;
	delimp=false;
	filler=false;
	fillrand=false;
	useinternal=false;
	fillerchar=0xFF;
	data=0;
	nCartLen=0;
	FILE_ANZ=0;
	oDirectoryPointer = 0x0380;
	oCardTroyan = 0;//0x0400;
	titleadr=0x2400;// default
	hackhead=0;
	writelyx=true;
	writelnx=true;

      strcpy(lnxmanu,"lynxdir (c) B.S.");
}

void lynxrom::SetBlockSize(int s)
{// s==256 ||
  if( s==512 || s==1024 || s==2048){
	  if(s!=blocksize){
		blocksize=s;
		init_rom(blocksize,blockcount);
	  }
  }else{
	  printf("Error: %d is no valid blocksize!\n",s);
  }
}

bool lynxrom::init_rom(int bs,int bc)
{
	if( data) delete []data;
	blocksize=bs;
	blockcount=bc;	
	nMaxSize=blocksize*blockcount;
	nCartLen=0;
	FILE_ANZ=0;
	
	data=new unsigned char[nMaxSize];
	
	memset( data, fillerchar, nMaxSize);
	
	if(fillrand){
		for (int i=0; i<nMaxSize; i++) data[i]=random()&0xFF;
	}
}

bool lynxrom::AddFile(char *fname,bool bootpic,bool blockalign,bool mode,int offset)
{
	if(fname && *fname){
		char *c=fname;
		while(*c){
			if( *c=='\\') *c='/';
			c++;	
		}
		FILES[FILE_ANZ].fname=new char[strlen(fname)+1];
		strcpy(FILES[FILE_ANZ].fname,fname);
		FILES[FILE_ANZ].filesize=FileLength(fname);
//		FILES[FILE_ANZ].filesize = ((FILES[FILE_ANZ].filesize + 1) & 0xfffffffe);// WORKAROUND... BUT WHY

	}else{
		FILES[FILE_ANZ].fname=0;
		FILES[FILE_ANZ].filesize=0;
	}
	FILES[FILE_ANZ].bootpic=bootpic;
	FILES[FILE_ANZ].blockalign=blockalign;
	FILES[FILE_ANZ].entrymode=mode;
	FILES[FILE_ANZ].newdiroffset=offset;
	FILES[FILE_ANZ].memory=0;
	FILES[FILE_ANZ].pointer=0;
	FILES[FILE_ANZ].inromsize=FILES[FILE_ANZ].filesize;
	FILES[FILE_ANZ].copyof=-1;
	FILES[FILE_ANZ].dirpointer=0;
	FILES[FILE_ANZ].loadadr=0;
	
	FILE_ANZ++;
	return(true);
}


bool lynxrom::AddCopy(int nr,bool mode,int offset)
{
  static char copyof[]="CopyOf";
	if( nr<0 || nr>=FILE_ANZ){
	  printf("Cannot reference to a future file (%d but i am %d)!\n",nr,FILE_ANZ);
	  exit(100);
	}

	FILES[FILE_ANZ].fname=copyof;
	FILES[FILE_ANZ].filesize=0;

	FILES[FILE_ANZ].entrymode=mode;
	FILES[FILE_ANZ].newdiroffset=offset;
	FILES[FILE_ANZ].memory=0;
	FILES[FILE_ANZ].pointer=0;
	FILES[FILE_ANZ].copyof=nr;
	FILES[FILE_ANZ].inromsize=0;
	FILES[FILE_ANZ].dirpointer=0;
	FILES[FILE_ANZ].loadadr=0;

	FILE_ANZ++;
	return(true);
}

bool lynxrom::savelyx(char *fn)
{
	if(!writelyx) return(true);

	FILE *fh;
	printf("Writing to %s\n",fn);
	fh=fopen(fn,"wb+");
	if(fh==0) return(false);
	
	if(filler) nCartLen=nMaxSize; 
	if(fwrite(data,1,nCartLen,fh)!=nCartLen) printf("Error: Couldn't write %s !\n",fn);
	
	fclose(fh);
	return(true);	
}

bool lynxrom::savelnx(char *fn)
{
	if(!writelnx) return(true);
	
	FILE *fh;
	struct LNX_STRUCT *ll;
	
	printf("Writing to %s\n",fn);
	fh=fopen(fn,"wb+");
	if(fh==0) return(false);
	
	ll=new struct LNX_STRUCT;
	
	memset(ll,0,sizeof(struct LNX_STRUCT));
	strcpy((char *)ll->magic,"LYNX");
	ll->page_size_bank0=blocksize;
	// ll->page_size_bank1=0;
	ll->version=1;
	char *bn;
	bn=strrchr(fn,'/');
	if(bn==0) bn=fn;
	bn=strrchr(fn,'\\');
	if(bn==0) bn=fn;
	strncpy((char *)ll->cartname,lnxname,32);
	((char *)ll->cartname)[31]=0;
	strncpy((char *)ll->manufname,lnxmanu,16);
	((char *)ll->manufname)[15]=0;
	ll->rotation=lnxrot;
	
	if(fwrite(ll,1,sizeof(struct LNX_STRUCT),fh)!=sizeof(struct LNX_STRUCT)) printf("Error: Couldn't write LN header for  %s !\n",fn);
	nCartLen=nMaxSize; 
	if(fwrite(data,1,nCartLen,fh)!=nCartLen) printf("Error: Couldn't write data for %s !\n",fn);
	
	fclose(fh);

	delete ll;
	return(true);
}

void lynxrom::copy_bll_header(void)
{
	// Header kopieren
	printf("Copy internal BLL newloader.\n");
	for (int i=0; i< 0x380; i++) data[i] = CardHead[i]; // 0x380 von 0x0408
	/*if( !checksum)*/{
		printf("Checksum disabled.\n");
		data[0x303] = 0x80; // Patch: BRA instead of BEQ --> No checksum required
		// remark by BS: Totally wrong, this is crctab-Adress!!!
		// very strange that this works!!!
	}
}

bool lynxrom::process_files(void)
{
	int ndirold;
	printf("\n--- Process %d Files ---\n\n", FILE_ANZ);
	ndirold=oDirectoryPointer;
	for(int i=0; i<FILE_ANZ; i++){
		if(verbose){
			printf("------ %d. entry in list ----------------------------\n", i);
		}
		ProcessFile(FILES[i]);
	}

	printf("\n");
	if(oCardTroyan>0){
	  if (oDirectoryPointer < oCardTroyan+8){	// Troyan Horse!!!
		  printf("Before Troyan %d = %xh -> adjust Offset\n", oDirectoryPointer,oDirectoryPointer);
		  oDirectoryPointer = oCardTroyan+8;
	  }
	}

	printf("Data start at Offset: %d = %xh\n", oDirectoryPointer,oDirectoryPointer);
	nCartLen = oDirectoryPointer;

	short nBlockNo;
	short nBlockOffset;
	nBlockOffset = nCartLen % blocksize;
	nBlockNo = nCartLen / blocksize;

	oDirectoryPointer=ndirold;
	printf("--- Directory processed ---\n\n");


	if(verbose){
		printf("*********************************************\n");
		printf("nCartLen: %d = %xh\n", nCartLen, nCartLen);
		printf("nDirOffset: %d = %xh\n", oDirectoryPointer,oDirectoryPointer);
		printf("Block %xh, Offset %xh\n", nBlockNo, nBlockOffset);
		printf("*********************************************\n");
	}
	return true;
}

void lynxrom::ProcessFile(struct FILE_PAR &file)
{
		printf("nDirOffset: %d = %xh\n", oDirectoryPointer,oDirectoryPointer);
		// Jetzt die Datei
		if(oDirectoryPointer==oCardTroyan){
			oDirectoryPointer += 8;// Troyan Horse!!!
			printf("Troyan Horse here!\n------- next entry ---\n");
		}

		if(file.newdiroffset>0){
		  if(file.newdiroffset<oDirectoryPointer){
			printf("Tried to set negative offset... new:%d < old:%d\n",file.newdiroffset,oDirectoryPointer);
		  }else{
			printf("Set new Directory position offset... (old:%d)\n",oDirectoryPointer);
			oDirectoryPointer=file.newdiroffset;
			printf("nDirOffset: %d = %xh\n", oDirectoryPointer,oDirectoryPointer);
		  }
		}

		if( file.fname==0){
			if(!file.bootpic){
				printf("\n empty filename -> bug!\n");
				exit(100);
			}
			printf("Title Pic (built-in)\n");
		}else{
			printf("%s\n",file.fname);
		}

		if(file.copyof>=0){
		  printf("COPY of entry %d\n",file.copyof);
		}
		file.dirpointer=oDirectoryPointer;
		oDirectoryPointer += 8;
}

bool lynxrom::add_files(void)
{
	int tt=0;
	struct FILE_PAR *file;
	
	printf("%d Files\n", FILE_ANZ);
	for(int i=0; i<FILE_ANZ; i++){
		short nBlockNo;
		short nBlockOffset;
		if(verbose){
			printf("------ %d. entry in list ----------------------------\n", i);
		}
		file=&FILES[i];

		nBlockOffset = nCartLen % blocksize;
		nBlockNo = nCartLen / blocksize;

		if(file->blockalign)
		{// Dann Block-Align durchf?hren
			nCartLen += blocksize-1;
			nCartLen &= ~(blocksize-1);
			printf("#ALIGN:\n");
			if(verbose){
				nBlockOffset = nCartLen % blocksize;
				nBlockNo = nCartLen / blocksize;
				printf("Next one will be at: Block %x, Offset %x\n", nBlockNo, nBlockOffset);
			}
		}
		
		if( file->fname==0){
			if(!file->bootpic){
				printf("\n empty filename -> bug!\n");
				continue;
			}
			
			file->memory=dummy_insert;
			file->filesize=DUMMY_INSERT_SIZE;
			file->loadadr=0x2400; // fixed adress for title pic

			printf("%d: Title Pic (built-in)\n",tt++);
		}else{
			printf("%d: %s\n",tt++,file->fname);
		}

		if(file->copyof>=0){
		  printf("COPY of entry %d\n",file->copyof);
		  continue;
		}
		file->inromloadoffset=nCartLen;
		if(file->memory==0){
		  if(LoadFile(file)==0){
			printf("Bug: File %s could not be loaded!\n",file->fname);
			exit(100);
		  }
		}
		AnalyseFile(file);// And write to Header
		if(WriteFileToRom(file)){
			if(verbose) printf(" size: %d (%x) bytes",file->inromsize,file->inromsize);
			nCartLen+=file->inromsize;

			printf("\n");
			if(verbose){
				nBlockOffset = nCartLen % blocksize;
				nBlockNo = nCartLen / blocksize;
				printf("File handled:\n");
				printf("Next one will be at:  %x, Offset %x\n", nBlockNo, nBlockOffset);
			}
			
		}else{
			printf("File NOT handled:\n");
			break;
		}
	}

	if(verbose){
	  short nBlockOffset;
	  short nBlockNo;
	  nBlockOffset = nCartLen % blocksize;
	  nBlockNo = nCartLen / blocksize;
		printf("*********************************************\n");
		printf("nCartLen: %d = %xh\n", nCartLen, nCartLen);
		printf("nDirOffset: %d = %xh\n", oDirectoryPointer,oDirectoryPointer);
		printf("Block %xh, Offset %xh\n", nBlockNo, nBlockOffset);
		printf("*********************************************\n");
	}

	if(FILES[0].loadadr==0){
		printf("Set Title Adress to %d ($%04X) as none was defined within title file header.\n",titleadr,titleadr);
		FILES[0].loadadr=titleadr; // fixed adress for title pic
	}

	for(int i=0; i<FILE_ANZ; i++){
		WriteDirEntry(&FILES[i]);
	}
}                                                                              

bool lynxrom::WriteFileToRom(struct FILE_PAR *file)
{
	if(nCartLen+file->inromsize>nMaxSize){
		printf("\nROM Size exceeded!\n");
		return(false);
	}
	memcpy(data+nCartLen,file->pointer,file->inromsize);
	return(true);
}

bool lynxrom::WriteDirEntry(struct FILE_PAR *file)
{
	unsigned char *pDirectoryPointer;
	int bo, bn, fl;

	if(file->copyof>=0){
	  WriteDirCopyEntry( &FILES[file->copyof], file->dirpointer, file->entrymode);
	  return true;
	}
	pDirectoryPointer=data+file->dirpointer;

	bo = file->inromloadoffset % blocksize;
	bn = file->inromloadoffset / blocksize;
	fl = file->indirfilesize;
	
	pDirectoryPointer[0] = bn;
	pDirectoryPointer[3] = file->flag;
	pDirectoryPointer[4] = file->loadadr & 0xff;	// Steht "falsch" rum drin
	pDirectoryPointer[5] = file->loadadr >> 8;
	if(file->entrymode){//EPYX
	  pDirectoryPointer[1] = ((bo ) & 0xff);
	  pDirectoryPointer[2] = ((bo ) >> 8);
	  pDirectoryPointer[6] = ((fl) & 0xff);
	  pDirectoryPointer[7] = ((fl) >> 8);
	}else{// BLL
	  pDirectoryPointer[1] = ((bo ^ 0xffff) & 0xff);
	  pDirectoryPointer[2] = ((bo ^ 0xffff) >> 8);
	  pDirectoryPointer[6] = ((fl ^ 0xffff) & 0xff);
	  pDirectoryPointer[7] = ((fl ^ 0xffff) >> 8);
	}
	  return true;
}

bool lynxrom::WriteDirCopyEntry(struct FILE_PAR *file,int dirpointer,bool mode)
{
	unsigned char *pDirectoryPointer;
	int bo, bn, fl;

	pDirectoryPointer=data+dirpointer;

	bo = file->inromloadoffset % blocksize;
	bn = file->inromloadoffset / blocksize;
	fl = file->indirfilesize;

	pDirectoryPointer[0] = bn;
	pDirectoryPointer[3] = file->flag;
	pDirectoryPointer[4] = file->loadadr & 0xff;	// Steht "falsch" rum drin
	pDirectoryPointer[5] = file->loadadr >> 8;
	if(mode){//EPYX
	  pDirectoryPointer[1] = ((bo ) & 0xff);
	  pDirectoryPointer[2] = ((bo ) >> 8);
	  pDirectoryPointer[6] = ((fl) & 0xff);
	  pDirectoryPointer[7] = ((fl) >> 8);
	}else{// BLL
	  pDirectoryPointer[1] = ((bo ^ 0xffff) & 0xff);
	  pDirectoryPointer[2] = ((bo ^ 0xffff) >> 8);
	  pDirectoryPointer[6] = ((fl ^ 0xffff) & 0xff);
	  pDirectoryPointer[7] = ((fl ^ 0xffff) >> 8);
	}
}

bool lynxrom::AnalyseFile(struct FILE_PAR *file)
{
	int nTmpFileLength;
	unsigned char fflag;
	
	file->pointer=file->memory;
	file->data=file->memory;
	file->inromsize=file->filesize;
	file->indirfilesize=file->inromsize;

	if( strncmp((const char *)file->memory,"ROMRIP",6)==0){
	  file->pointer+=16;
	  file->inromsize-=16;
	  file->indirfilesize-=16;
	  file->data+=16;
	  file->flag = ((unsigned char *)file->memory)[8+2];
	  file->loadadr = ((unsigned short *)file->memory)[8/2];
	  printf("RomRip ");
	}else{
	  fflag = file->memory[0] | file->memory[1];
	  if ((fflag == 0x88) ||	// Normales Programm
		  (fflag == 0x89))	// Gepacktes Programm
	  {
		  // Korrigierten Offset
		  if(skipheader){
			  file->pointer+=HEADERLEN;
			  file->inromsize-=HEADERLEN;
		  }else{
			file->inromloadoffset+=HEADERLEN;
		  }
		  file->indirfilesize-=HEADERLEN;
		  file->data+=HEADERLEN;


		  file->flag = fflag;
		  file->loadadr = file->memory[2]*256 + file->memory[3];// Startadresse

		  if (fflag == 0x89)	// Gepacktes Prg.
		  {
			  file->indirfilesize = file->memory[4]*256 + file->memory[5];
			  // TP packed program ... length in dir is unpacked(!) length not packed length
			  printf("TP Exec ");
		  }else{
			  printf("Exec ");
		  }
	  }
	
	// Check if we have IMP! File
	  if( file->data[0]=='I' && file->data[1]=='M' && file->data[2]=='P' && file->data[3]=='!'){
		  file->flag = 'I';// Data imploded
		  printf("Imploded ");
		  if(delimp){// ... and remove Magic
			  file->data[0]=random()&0xFF;
			  file->data[1]=random()&0xFF;
			  file->data[2]=random()&0xFF;
			  file->data[3]=random()&0xFF;
		  }
	  }
	}
	// noPrg
	
	return(true);
}

bool lynxrom::built(void)
{
	if(useinternal) copy_bll_header();
	process_files();
	add_files();
	if(oCardTroyan>0){
	  printf("Now write troyan entry\n");
	  unsigned char troy[]={0,0,2,0,0,2,0,2};
	  memcpy(data+oCardTroyan,troy,8);
	}
	printf("==> Size: %d %x \n",nCartLen,nCartLen);
	if(hackhead){
	  printf("=== Set Hacked encrypted header\n");
	  switch(hackhead){
		case 512:
		  printf("=== type 512\n");
		  memcpy(data,hackload_stage1,154);
		  memcpy(data+154,hackload_stage2_512,256);
		  break;
		case 1024:
		  printf("=== type 1024\n");
		  memcpy(data,hackload_stage1,154);
		  memcpy(data+154,hackload_stage2_1024,256);
		  break;
		case 2048:
		  printf("=== Type 2048\n");
		  memcpy(data,hackload_stage1,154);
		  memcpy(data+154,hackload_stage2_2048,256);
		  break;
		default:
		  printf("=== Block size not supported!!!\n");
		  break;
	  }
	}
}
