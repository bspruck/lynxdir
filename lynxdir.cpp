/*
  Completly rewritten by BS
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>

#define VER "1.0"

#define stricmp(a,b)	strcasecmp(a,b)
#define strnicmp(a,b,c)	strncasecmp(a,b,c)

#include "lynxrom.h"

// #define DEBUG 1

bool verbose;

lynxrom ROM;
char *ROMname;

bool ExtractName(char *fname)
{
	int l;
	char *c, *d;
	l=strlen(fname);
	ROMname=new char[l+5];
	strcpy(ROMname,fname);
	c=strrchr(ROMname,'.');
	d=strrchr(ROMname,'/');
	
	if( c && c>d) *c=0;
}

bool ParseMAK(char *fname)
{
	char buffer[1000];
	FILE *fh;
	bool align=false, mode=false, title=true;
	int offset=0;
	int fileadr=0;
	
	fh=fopen(fname,"rt");
	if(fh==0) return(false);
	
	while(!feof(fh)){
		char *c;
		*buffer=0;
		fgets(buffer,999,fh);
		buffer[999]=0;

		if(buffer[0]==0 || buffer[0]==';') continue;

		c=buffer;
		while(*c) c++;
		while(c>buffer){
			if( *c==0 || *c==10 || *c==13 || *c==' ' || *c=='\t'){
				*c=0;
				c--;
				continue;
			}
			break;
		}
		
		c=buffer;
		while( *c==' ' || *c=='\t') c++;
		
		if(*c=='#'){// Options
			/// ROM file option like:
			/// BLOCKSIZE nr
			/// DIRSTART nr
			/// TROYAN (put troyan horse as file 16) BLL 1024k

			/// next FILE options
			/// ALIGN
			/// TITLE
			/// BLL
			/// EPYX
			/// COPY nr
			if(strnicmp(c+1,"BLOCKSIZE",9)==0){
				ROM.SetBlockSize(atoi(c+10));
			}else if(strnicmp(c+1,"DIRSTART",8)==0){
				ROM.SetDirStart(atoi(c+9));
			}else if(strnicmp(c+1,"DIROFFSET",9)==0){
				offset=atoi(c+10);
			}else if(strnicmp(c+1,"HACK512",7)==0){
				printf("-> Set HACK512 header\n");
				ROM.SetHackHeader(512);
			}else if(strnicmp(c+1,"HACK1024",8)==0){
				printf("-> Set HACK1024 header\n");
				ROM.SetHackHeader(1024);
			}else if(strnicmp(c+1,"HACK2048",8)==0){
				printf("-> Set HACK2048 header\n");
				ROM.SetHackHeader(2048);
			}else if(strnicmp(c+1,"TROYAN",6)==0){
				printf("-> Set troyan entry (FILE 16)\n");
				ROM.SetTroyan();
			}else if(strnicmp(c+1,"INTERNAL",8)==0){
				printf("-> Use internal bll newloader\n");
				ROM.SetInternalLoader();
			}else if(strnicmp(c+1,"TITLEADR",8)==0){
				printf("-> Set adress for titlepic\n");
				ROM.SetTitleAdr(atoi(c+9));
			}else if(strnicmp(c+1,"PUTTITLE",8)==0){
				printf("-> Use internal titlepic\n");
				ROM.AddFile(0,true,align,mode,offset);// fileadr
				align=false;
				title=false;
				offset=0;
			}else if(strnicmp(c+1,"FILEADR",7)==0){
				fileadr=atoi(c+8);
				printf("-> Set Default Load Adr for data files\n");
			}else if(strnicmp(c+1,"ALIGN",5)==0){
				printf("-> Align next\n");
				align=true;
			}else if(strnicmp(c+1,"BLL",3)==0){
				printf("-> Set Dir Mode Bll\n");
				mode=false;
			}else if(strnicmp(c+1,"EPYX",4)==0){
				printf("-> Set Dir Mode Epyx\n");
				mode=true;
			}else if(strnicmp(c+1,"COPY",4)==0){
				ROM.AddCopy(atoi(c+5),mode,offset);// fileadr
				align=false;
				title=false;
				offset=0;
			}
		}else{// Filename
			ROM.AddFile(c,title,align,mode,offset);// fileadr
			align=false;
			title=false;
			offset=0;
		}
	}
	fclose(fh);
	return(true);
}

char usage[]={
	"\nUsage :\n"
	"lynxdir [-hvsif01r] batchfile.mak\n"
	"-h this help\n"
	"-v verbose\n" 
	"-f[01r] fill ROM with 0,1,random\n"
	"-s dont skip exec header (adds extra 10 bytes per exec)\n"
	"-i remove imp (overwrite with random)\n"
	"all other options should be set in MAK file\n"
};

/*************************************************************
*** Main                                                   ***
*************************************************************/
int main(int argc,char *argv[])
{
	printf("----------------------------------------\n"
	"LynxDir Generator Version "VER"\n"
	" a replacement for the lynxer \n"
	"(c) 2010 Bjoern Spruck\n"
	" based on the lynxer by Bastian Schick\n"
	" It can create ROMs with \n"
	" * BLL type 1024 bytes/block, using Troyan Horse\n"
	" * EPXY Loader 512/1024/2048 w/o checksum\n"
	" * EPXY Loader with BLL type file system\n"
	" * without loader for later encryption\n"
	"----------------------------------------\n");
	
	if (argc == 1 )
	{
		printf("%s:\n%s",argv[0],usage);
		exit(-1);
	}

	// Handle Parameters

	ROM.init();
	ROM.init_rom(1024,256);
	
	int argc_filename;

	verbose=false;
	
	argc_filename=1;
	for(int ii=1; ii<argc; ii++){
		if( argv[ii][0]!='-') break;
		for(int jj=1; jj<10; jj++){
			if(argv[ii][jj]==0) break;
			switch(argv[ii][jj]){
				case 'h':{
					printf("%s:\n%s",argv[0],usage);
					exit(-1);
				}
				case 'v':{
					verbose=true;
					ROM.set_verbose(true);
					break;
				}
				case 'f':{
					ROM.set_filler(true);
					printf("Fill LYX!\n");
					break;
				}
				case '0':{					
					ROM.set_filler_zero();
					printf("Fill with 0x00!\n");
					break;
				}
				case '1':{					
					ROM.set_filler_one();
					printf("Fill with 0xFF!\n");
					break;
				}
				case 'r':{
					ROM.set_fillrand(true);
					printf("Fill with random!\n");
					break;
				}
				case 's':{
					ROM.set_skipheader(false);
					printf("Dont Skip Header!\n");
					break;
				}
				case 'i':{
					ROM.set_delimp(true);
					printf("Remove IMP!\n");
					break;
				}
				default:{
					printf("\nwrong parameter %s\n",argv[ii]);
					exit(1);
					break;
				}
			}
		}
		argc_filename++;
	}
	
	if( argc<=argc_filename){
		printf("\nfilename missing\n");
		exit(1);
	}
	
	// Check if File is homebrew.o or MAK
	
	{
		FILE *fh;
		fh=fopen(argv[argc_filename],"rb");
		if(!fh){
			printf("\nCannot read %s.\n",argv[argc_filename]);
			exit(1);
		}
		
		ExtractName(argv[argc_filename]);
		unsigned char header[4];
		fread(header,1,4,fh);
		fclose(fh);
		if((header[0]|header[1])==0x88){
			// HOMEBREW.O
			printf("\n%s is homebrew.\n",argv[argc_filename]);
			printf("\n\"simple\" mode not supported!\n",argv[argc_filename]);
			exit(100);
		}else if((header[0]|header[1])==0x89){
			// HOMEBREW.O but Packed :-(
			printf("\n%s is a turbopacked file, not supported for \"simple\" mode.\n",argv[argc_filename]);
			exit(100);
		}else{
			// Seems to be MAK
			printf("\nRunning in script mode. (%s)\n",argv[argc_filename]);
			if(!ParseMAK(argv[argc_filename])){
				printf("\nProblems loading file. %s\n",argv[argc_filename]);
				exit(1);			
			}
			printf("\nMak file parsed. (%s)\n",argv[argc_filename]);
		}
	}

	printf("\nBuilding ROM...\n");
	ROM.built();
	strcat(ROMname,".lyx");
	printf("Writing to %s\n",ROMname);
	ROM.savelyx(ROMname);
	return(0);
}

