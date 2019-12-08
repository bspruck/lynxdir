/*
  Completly rewritten by BS
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>

#define VER "1.9.3"

#define stricmp(a,b)  strcasecmp(a,b)
#define strnicmp(a,b,c) strncasecmp(a,b,c)

#include "lynxrom.h"

// #define DEBUG 1

#define MODE_BLL    false
#define MODE_EPICS  true

bool verbose;

lynxrom ROM;
char* ROMname;

bool ExtractName(char* fname)
{
  int l;
  char* c, *d;
  l = strlen(fname);
  ROMname = new char[l + 5];
  strcpy(ROMname, fname);
  c = strrchr(ROMname, '.');
  d = strrchr(ROMname, '/');

  if (c && c > d) *c = 0;
}

bool ParseMAK(char* fname)
{
  char buffer[1000];
  FILE* fh;
  bool align = false, mode = MODE_BLL, title = true, skip_bank = false, cont_bank=false;
  int offset = 0;
  int fileadr = 0;
  int addfileoffset = 0;

  {
    char* c, nn[65];
    c = strrchr(fname, '/');
    if (c == 0) c = fname;
    strncpy(nn, c, 64);
    nn[64] = 0;
    c = strrchr(nn, '.');
    if (c) *c = 0;
    ROM.set_lnxname(nn);
  }


  fh = fopen(fname, "rt");
  if (fh == 0) return (false);

  while (!feof(fh)) {
    char* c;
    *buffer = 0;
    fgets(buffer, 999, fh);
    buffer[999] = 0;

    if (buffer[0] == 0 || buffer[0] == ';') continue;

    c = buffer;
    while (*c) c++;
    while (c > buffer) {
      if (*c == 0 || *c == 10 || *c == 13 || *c == ' ' || *c == '\t') {
        *c = 0;
        c--;
        continue;
      }
      break;
    }

    c = buffer;
    while (*c == ' ' || *c == '\t') c++;

    if (*c == '#') { // Options
      int newfileoffset;
      newfileoffset = 0;
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
      if (strnicmp(c + 1, "BLOCKSIZE", 9) == 0) {
        ROM.SetBlockSize(atoi(c + 10));
      } else if (strnicmp(c + 1, "DIRSTART", 8) == 0) {
        ROM.SetDirStart(atoi(c + 9));
      } else if (strnicmp(c + 1, "DIROFFSET", 9) == 0) {
        offset = atoi(c + 10);
      } else if (strnicmp(c + 1, "NOLOADER", 8) == 0) {
        printf("-> Dont write any loader, fill with 0s\n");
        ROM.SetLoader(L_NOLOADER);
      } else if (strnicmp(c + 1, "NEWMINI_F000", 12) == 0) {
        printf("-> Set new Minimal header (+ loader) @ $F000\n");
        mode = MODE_EPICS;
        ROM.SetMiniHeader(L_MINI_F000);
      } else if (strnicmp(c + 1, "NEWMINI_FB68", 12) == 0) {
        printf("-> Set new Minimal header (+ loader) @ $FB68\n");
        mode = MODE_EPICS;
        ROM.SetMiniHeader(L_MINI_FB68);
      } else if (strnicmp(c + 1, "HACKAUTO", 8) == 0) {
        printf("-> Set HACK header (auto determine)\n");
        mode = MODE_EPICS;
        ROM.SetHackHeader(L_HACKAUTO, 0);
      } else if (strnicmp(c + 1, "HACK512", 7) == 0) {
        printf("-> Set HACK512 header\n");
        mode = MODE_EPICS;
        ROM.SetHackHeader(L_HACK512, 512);
      } else if (strnicmp(c + 1, "HACK1024", 8) == 0) {
        printf("-> Set HACK1024 header\n");
        mode = MODE_EPICS;
        ROM.SetHackHeader(L_HACK1024, 1024);
      } else if (strnicmp(c + 1, "HACK2048", 8) == 0) {
        printf("-> Set HACK2048 header\n");
        mode = MODE_EPICS;
        ROM.SetHackHeader(L_HACK2048, 2048);
      } else if (strnicmp(c + 1, "INTERNAL", 8) == 0) {
        printf("-> Use internal bll newloader\n");
        mode = MODE_BLL;
        ROM.SetInternalLoader();
      } else if (strnicmp(c + 1, "AUDIN", 5) == 0) {
        printf("-> Set AUDIN cart\n");
        ROM.SetAudIn(true);
      } else if (strnicmp(c + 1, "BANK2", 5) == 0) {
        printf("-> Set BANK2 cart\n");
        ROM.SetBank2(true);
      } else if (strnicmp(c + 1, "CONT", 4) == 0) {
        printf("-> Set Continous Bank\n");
        ROM.SetContinueBank(true);
      } else if (strnicmp(c + 1, "NOLYX", 5) == 0) {
        printf("-> Dont write LYX\n");
        ROM.set_write_lyx(false);
      } else if (strnicmp(c + 1, "NOLNX", 5) == 0) {
        printf("-> Dont write LNX\n");
        ROM.set_write_lnx(false);
      } else if (strnicmp(c + 1, "LNXROT", 6) == 0) {
        int rotation;
        rotation = atoi(c + 7);
        printf("-> Flag Rotate in LNX to ");
        switch (rotation) {
          case 0: printf("NORM\n"); break;
          case 1: printf("LEFT\n"); break;
          case 2: printf("RIGHT\n"); break;
          default: printf("undefined\n"); break;
        }
        ROM.set_lnxrot(rotation);
      } else if (strnicmp(c + 1, "LNXNAME", 7) == 0) {
        printf("-> Set Name of Game in LNX %s\n", c + 9);
        ROM.set_lnxname(c + 9);
      } else if (strnicmp(c + 1, "LNXMANU", 7) == 0) {
        printf("-> Set Manufacturer in LNX %s\n", c + 9);
        ROM.set_lnxmanu(c + 9);
      } else if (strnicmp(c + 1, "TROYAN", 6) == 0) {
        printf("-> Set troyan entry (FILE 16)\n");
        ROM.SetTroyan();
      } else if (strnicmp(c + 1, "TITLEADR", 8) == 0) {
        printf("-> Set adress for titlepic\n");
        ROM.SetTitleAdr(atoi(c + 9));
      } else if (strnicmp(c + 1, "PUTTITLE", 8) == 0) {
        printf("-> Use internal titlepic\n");
        ROM.AddFile(0, true, align, mode, offset, skip_bank); // fileadr
        align = false;
        title = false;
        skip_bank = false;
        offset = 0;
      } else if (strnicmp(c + 1, "FILEADR", 7) == 0) {
        fileadr = atoi(c + 8);
        printf("(not fully supported option!!!) -> Set Default Load Adr for data files to %d\n", fileadr);
      } else if (strnicmp(c + 1, "ALIGN", 5) == 0) {
        printf("-> Align next\n");
        align = true;
      } else if (strnicmp(c + 1, "SKIP_BANK", 9) == 0) {
        printf("-> Skip Bank next\n");
        skip_bank = true;
      } else if (strnicmp(c + 1, "BLL", 3) == 0) {
        printf("-> Set Dir Mode Bll\n");
        mode = MODE_BLL;
      } else if (strnicmp(c + 1, "EPYX", 4) == 0) {
        printf("-> Set Dir Mode Epyx\n");
        mode = MODE_EPICS;
      } else if (strnicmp(c + 1, "COPY", 4) == 0) {
        int nr=-2;
        if(sscanf(c+5,"%d",&nr)!=1) nr=-2;// reference last one 
        ROM.AddCopy( nr, mode, offset); // fileadr
        align = false;
        title = false;
        offset = 0;
      } else if (strnicmp(c + 1, "EMPTY", 5) == 0 || strnicmp(c + 1, "NONE", 4) == 0) {
        ROM.AddCopy(-1, mode, offset); // fileadr we fake a copy of
        align = false;
        title = false;
        offset = 0;
      } else if (strnicmp(c + 1, "SKIP", 4) == 0) {
        newfileoffset = atoi(c + 5);
        align = false;
        title = false;
        offset = 0;
      } else if (strnicmp(c + 1, "EE_8BIT", 7) == 0) {
        ROM.set_eeprom_8bit();
      } else if (strnicmp(c + 1, "EE_16BIT", 8) == 0) {
        ROM.set_eeprom_16bit();
      } else if (strnicmp(c + 1, "EE_93C46", 8) == 0) {
        ROM.set_eeprom_type(EEPROM_93C46);
      } else if (strnicmp(c + 1, "EE_93C56", 8) == 0) {
        ROM.set_eeprom_type(EEPROM_93C56);
      } else if (strnicmp(c + 1, "EE_93C66", 8) == 0) {
        ROM.set_eeprom_type(EEPROM_93C66);
      } else if (strnicmp(c + 1, "EE_93C76", 8) == 0) {
        ROM.set_eeprom_type(EEPROM_93C76);
      } else if (strnicmp(c + 1, "EE_93C86", 8) == 0) {
        ROM.set_eeprom_type(EEPROM_93C86);          
      }else{
        printf("== ERROR ===\nUnknown line \"%s\"\n",c);
        exit(1000);
      }
      addfileoffset = newfileoffset;
    } else if(*c==0 || *c==';' || *c==10 || *c==13){
      // Skip empty / comment line
    } else {
      ROM.AddFile(c, title, align, mode, offset, skip_bank, addfileoffset); // fileadr
      align = false;
      title = false;
      skip_bank = false;
      offset = 0;
      addfileoffset = 0;
    }
  }
  fclose(fh);
  return (true);
}

char usage[] = {
  "\nUsage :\n"
  "lynxdir [-hvsif01r] batchfile.mak\n"
  "lynxdir mainexe.o\n"
  "-h this help\n"
  "-v verbose\n"
  "-f[01r] fill ROM with 0,1,random\n"
  "-s dont skip exec header (adds extra 10 bytes per exec)\n"
  "-i remove imp (overwrite with random)\n"
  "-x dont write LNX\n"
  "-y dont write LYX\n"
  "all other options should be set in MAK file, see examples!\n"
};

bool add_lnx_header(const char* fn2, int len)
{
  struct LNX_STRUCT* ll;

  char* m = new char[len];

  FILE* fh;
  fh = fopen(fn2, "rb");
  if (!fh) {
    printf("\nCannot read %s.\n", fn2);
    exit(1);
  }
  fread(m, 1, len, fh);
  fclose(fh);

  char* fn;
  fn = new char[strlen(fn2) + 5];
  strcpy(fn, fn2);

  char* bn;
  bn = strrchr(fn, '.');
  if (bn) strcpy(bn, ".lnx");
  else strcat(bn, ".lnx");

  printf("Writing to %s\n", fn);
  fh = fopen(fn, "wb+");
  if (fh == 0) return (false);

  ll = new struct LNX_STRUCT;

  memset(ll, 0, sizeof(struct LNX_STRUCT));
  strcpy((char*)ll->magic, "LYNX");

  ll->page_size_bank0 = len >> 8;
  printf("using Blocksize of %d bytes\n", ll->page_size_bank0);
  // ll->page_size_bank1=0;

  ll->version = 1;
  bn = strrchr(fn, '/');
  if (bn == 0) bn = fn;
  bn = strrchr(fn, '\\');
  if (bn == 0) bn = fn;
  strncpy((char*)ll->cartname, bn, 32);
  ((char*)ll->cartname)[31] = 0;
  strncpy((char*)ll->manufname, "lynxdir (c) B.S.", 16);
  ((char*)ll->manufname)[15] = 0;
  // ll->rotation=0;

  if (fwrite(ll, 1, sizeof(struct LNX_STRUCT),
             fh) != sizeof(struct LNX_STRUCT)) printf("Error: Couldn't write LNX header for  %s !\n", fn);
  if (fwrite(m, 1, len, fh) != len) printf("Error: Couldn't write data for %s !\n", fn);

  fclose(fh);

  delete []m;
  delete ll;
  return (true);
}

/*************************************************************
*** Main                                                   ***
*************************************************************/
int main(int argc, char* argv[])
{
  printf("----------------------------------------\n"
         "LynxDir Generator Version " VER "\n"
         " a replacement for the lynxer \n"
         "(c) 2010-2017 Bjoern Spruck\n"
         " based on the lynxer by Bastian Schick\n"
         " It can create ROMs with \n"
         " * BLL type 1024 bytes/block, using Troyan Horse\n"
         " * EPXY Loader 512/1024/2048 w/o checksum\n"
         " * EPXY Loader with BLL type file system\n"
         " * CC65/Karri MiniLoader @ $F000 and $FB68\n"
         " * or without loader for later encryption\n"
         " ... and a few other things\n"
         "----------------------------------------\n");

  if (argc == 1) {
    printf("%s:\n%s", argv[0], usage);
    exit(-1);
  }

  // Handle Parameters

  ROM.init();
  ROM.init_rom(0, 256, false, false);

  int argc_filename;

  verbose = false;

  argc_filename = 1;
  for (int ii = 1; ii < argc; ii++) {
    if (argv[ii][0] != '-') break;
    for (int jj = 1; jj < 10; jj++) {
      if (argv[ii][jj] == 0) break;
      switch (argv[ii][jj]) {
        case 'h': {
          printf("%s:\n%s", argv[0], usage);
          exit(-1);
        }
        case 'v': {
          verbose = true;
          ROM.set_verbose(true);
          break;
        }
        case 'f': {
          ROM.set_filler(true);
          printf("Fill LYX!\n");
          break;
        }
        case '0': {
          ROM.set_filler_zero();
          printf("Fill with 0x00!\n");
          break;
        }
        case '1': {
          ROM.set_filler_one();
          printf("Fill with 0xFF!\n");
          break;
        }
        case 'r': {
          ROM.set_fillrand(true);
          printf("Fill with random!\n");
          break;
        }
        case 's': {
          ROM.set_skipheader(false);
          printf("Dont Skip Header!\n");
          break;
        }
        case 'i': {
          ROM.set_delimp(true);
          printf("Remove IMP!\n");
          break;
        }
        case 'x': {
          ROM.set_write_lnx(false);
          printf("Dont write LNX!\n");
          break;
        }
        case 'y': {
          ROM.set_write_lyx(false);
          printf("Dont write LYX!\n");
          break;
        }
        default: {
          printf("\nwrong parameter %s\n", argv[ii]);
          exit(1);
          break;
        }
      }
    }
    argc_filename++;
  }

  if (argc <= argc_filename) {
    printf("\nfilename missing\n");
    exit(1);
  }

  // Check if File is homebrew.o, rom image or MAK

  {
    unsigned int len;
    len = ROM.FileLength(argv[argc_filename]);

    FILE* fh;
    fh = fopen(argv[argc_filename], "rb");
    if (!fh) {
      printf("\nCannot read %s.\n", argv[argc_filename]);
      exit(1);
    }
    ExtractName(argv[argc_filename]);
    unsigned char header[4];
    fread(header, 1, 4, fh);
    fclose(fh);
    if ((len == 128 * 1024 || len == 256 * 1024 || len == 512 * 1024)) {
      printf("\n%s is a ROM image. The only thing I could do with that is remove the checksumming and add a LNX header...\n",
             argv[argc_filename]);
      printf("BUT!!! removing checksumming is not working automatically for all ROMS, therefore, only a LNX header will be added!\n"\
             "You might consider using make_lnx for this task next time!\n");
      add_lnx_header(argv[argc_filename], len);
      exit(0);
    } else if (strcasestr(argv[argc_filename], ".lyx")) {
      printf("\n%s is a ROM image. The only thing I could do with that is remove the checksumming and add a LNX header...\n",
             argv[argc_filename]);
      printf("BUT!!! removing checksumming is not working automatically for all ROMS\n" \
             "BUT!!! the lyx file has not a vaild size and not info on teh blocksize, therefore nothing can be done!\n"\
             "You might consider using make_lnx for this task, but you have to know the blocksize!\n"\
             "=> I will quit here!\n");
      exit(100);
    } else if ((len == 128 * 1024 + sizeof(struct LNX_STRUCT) || len == 256 * 1024 + sizeof(struct LNX_STRUCT)
                || len == 512 * 1024 + sizeof(struct LNX_STRUCT)) ||
               (header[0] == 'L' && header[1] == 'Y' && header[2] == 'N' && header[3] == 'X') ||  strcasestr(argv[argc_filename], ".lyx")) {
      printf("\n%s is a LNX image. The only thing I could do with that is removing the checksumming...\n", argv[argc_filename]);
      printf("BUT!!! this might not work for every ROM, therefore...\n=> I will quit here!\n");
      exit(100);
    } else if ((header[0] | header[1]) == 0x88) {
      // HOMEBREW.O
      printf("\n%s is BLL homebrew.\nusing \"simple\" mode!\n", argv[argc_filename]);

      // Shorter and without title picture ... karris cc65 loader to $f000 or $fb68
      ROM.SetBlockSize(512);
      ROM.SetDirStart(203);
      ROM.SetMiniHeader(2);// better use the higher one to allow very large binaries
      ROM.AddFile(argv[argc_filename], false, false, true, 0, 0);

      // Hacked epyxloader
//      ROM.SetBlockSize(512);
//      ROM.SetDirStart(410);
//      ROM.SetHackHeader(512);
//      ROM.AddFile(0,true,false,true,0,0);
//      ROM.AddFile(argv[argc_filename],false,false,true,0,0);

    } else if ((header[0] | header[1]) == 0x89) {
      // HOMEBREW.O but Packed :-(
      printf("\n%s is a turbopacked file, not supported for \"simple\" mode.\n", argv[argc_filename]);
      exit(100);
    } else {
      // Seems to be MAK
      printf("\nRunning in script mode. (%s)\n", argv[argc_filename]);
      if (!ParseMAK(argv[argc_filename])) {
        printf("\nProblems loading file. %s\n", argv[argc_filename]);
        exit(1);
      }
      printf("\nMak file parsed. (%s)\n", argv[argc_filename]);
    }
  }

  printf("\nBuilding ROM...\n");
  ROM.built();

  char* n;
  n = new char[strlen(ROMname) + 5];
  strcpy(n, ROMname);
  strcat(n, ".lyx");
  ROM.savelyx(n);
  strcpy(n, ROMname);
  strcat(n, ".lnx");
  ROM.savelnx(n);
  delete []n;
  return (0);
}

