#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>

#include "lynxrom.h"

//#define CARDHEADLEN 2048
#define HEADERLEN 10

// unsigned char CardHead[CARDHEADLEN];   // 1032
#include "loader27.h"
#include "dinsert.h"

#include "hack_l1.h"
#include "hack_l2_512.h"
#include "hack_l2_1024.h"
#include "hack_l2_2048.h"

#include "micro_loader.h"

// 52 bytes of first stage
// #include "menu_lex.h"  // LEX not working ...
// second stage...
//   run $F000
//   bra .go
// dirofflo   dc.b 0
// diroffhi   dc.b 0
// blockbc    dc.b 0
// epyxflag dc.b 0
// .go
//  lda #MENUX_FILE
/// Offset 2: dirofflo
/// Offset 3: diroffhi
/// Offset 4: blockbc
/// Offset 5: epyxflag
/// Offset 7: load file

// at least 410 + two directory entries
#define AUDIN_OFFSET_HACK  (410+2*8)
// at least 203 + one directory entry
#define AUDIN_OFFSET_MICRO  (203+8)

// geht nur fuer MICRO
int AUDIN_OFFSET=0;

struct LNX_STRUCT lnxhead;

bool audin_flag = false;

/*************************************************************
*** FileLength                                             ***
*** Returns the filelength                                 ***
*************************************************************/
unsigned long lynxrom::FileLength(char* fn)
{
  unsigned long len;
  FILE* fh;

  fh = fopen(fn, "rb");
  if (fh == 0) return (0);

  fseek(fh, 0L, SEEK_END);
  len = ftell(fh);
  fclose(fh);
  return (len);
}

/*************************************************************
*** LoadFile                                               ***
*************************************************************/
unsigned long lynxrom::LoadFile(struct FILE_PAR* file)
{
  FILE* fh;

  if (file->fname == 0) return (0);
  fh = fopen(file->fname, "rb");
  if (fh == 0) return (0);

  file->memory = new unsigned char[file->filesize];
  file->memory[file->filesize - 1] = 0xFF;
  if (file->filesize != fread(file->memory, 1, file->filesize, fh)) {
    printf("%s: File Size changed??\n", file->fname);
  }
  fclose(fh);

  return (file->filesize);
}


void lynxrom::init(void)
{
  verbose = 0;
  skipheader = true;
  delimp = false;
  filler = false;
  fillrand = false;
  fillerchar = 0xFF;
  data = 0;
  
  loader=L_UNDEF;

  audin = false;
  bank2 = false;
  contcont = false;
  eeprom_type= NO_EEPROM;
  eeprom_8bit=false;
  
  blocksize = 0;
  blockcount = 256;

  nCartLen = 0;
  FILE_ANZ = 0;
  oDirectoryPointer = 0; // 0x0380;
  oCardTroyan = 0;//0x0400;
  titleadr = 0x2400; // default
  writelyx = true;
  writelnx = true;
  lnxrot = 0;

  strcpy(lnxmanu, "lynxdir (c) B.S.");
}

void lynxrom::SetBlockSize(int s)
{
  // s==256 ||
  if (s == 512 || s == 1024 || s == 2048) {
    if (s != blocksize) {
      blocksize = s;
      init_rom(blocksize, blockcount, audin, bank2);
    }
  } else {
    printf("Error: %d is no valid blocksize!\n", s);
  }
}

void lynxrom::SetBlockLimit(int limit)
{
  if( limit>0 && limit<=256){
    blockcount=limit;
    init_rom(blocksize, blockcount, audin, bank2);
  } else {
    printf("Error: %d is no valid blocklimit!\n", limit);
  }
}

void lynxrom::SetAudIn(bool flag)
{
  init_rom(blocksize, blockcount, flag, bank2);
}

void lynxrom::SetBank2(bool flag)
{
  init_rom(blocksize, blockcount, audin, flag);
}

void lynxrom::SetContinueBank(bool flag)
{
  contcont=flag;
}

void lynxrom::init_rom(int bs, int bc, int ai, int b2)
{
  if (data) delete []data;
  blocksize = bs;
  blockcount = bc;
  audin = ai;
  bank2 = b2;
  if (ai || b2) {
    if (ai && b2) {
      if(verbose) printf("AUDIN and BANK2: blockcount *4 \n");
      blockcount *=4;
    } else {
      if(verbose) printf("AUDIN or BANK2: blockcount *2 \n");
      blockcount *=2;
    }
  }
  nMaxSize = blocksize * blockcount;
  // if(verbose)
  if(verbose) printf("Init ROM Block Size %d Block Limit %d Overall Size %d \n", blocksize, blockcount, nMaxSize);
  nCartLen = 0;
  FILE_ANZ = 0;

  if(nMaxSize==0){
      data=0;
      return;
  }
  data = new unsigned char[nMaxSize];

  memset(data, fillerchar, nMaxSize);

  if (fillrand) {
    for (int i = 0; i < nMaxSize; i++) data[i] = random() & 0xFF;
  }
}

bool lynxrom::AddFile(char* fname, bool bootpic, bool blockalign, bool mode, int offset, bool skip_bank, int addoff,int loadadr)
{
    if(FILE_ANZ==256){
        printf("WARNING: More than 256 files!\n");
    }
  // mode: true EPYX, false BLL
  if (fname && *fname) {
    char* c = fname;
    while (*c) {
      if (*c == '\\') *c = '/';
      c++;
    }
    FILES[FILE_ANZ].fname = new char[strlen(fname) + 1];
    strcpy(FILES[FILE_ANZ].fname, fname);
    FILES[FILE_ANZ].filesize = FileLength(fname);
//    FILES[FILE_ANZ].filesize = ((FILES[FILE_ANZ].filesize + 1) & 0xfffffffe);// WORKAROUND... BUT WHY

  } else {
    FILES[FILE_ANZ].fname = 0;
    FILES[FILE_ANZ].filesize = 0;
  }
  FILES[FILE_ANZ].bootpic = bootpic;
  FILES[FILE_ANZ].blockalign = blockalign;
  FILES[FILE_ANZ].skip_bank = skip_bank;
  FILES[FILE_ANZ].entrymode = mode;
  FILES[FILE_ANZ].newdiroffset = offset;
  FILES[FILE_ANZ].memory = 0;
  FILES[FILE_ANZ].pointer = 0;
  FILES[FILE_ANZ].inromsize = FILES[FILE_ANZ].filesize;
  FILES[FILE_ANZ].copyof = -99;
  FILES[FILE_ANZ].dirpointer = 0;
  FILES[FILE_ANZ].loadadr = loadadr;
  FILES[FILE_ANZ].addfileoffset = addoff;

  FILE_ANZ++;
  return (true);
}

bool lynxrom::AddCopy(int nr, bool mode, int offset)
{
  static char copyof[] = "CopyOf";
  static char nocopy[] = "Empty";
  if (nr < -2 || nr >= FILE_ANZ) {
    printf("Cannot reference to a future file (%d but i am %d)!\n", nr, FILE_ANZ);
    exit(100);
  }

  if(nr==-2) nr=FILE_ANZ-1;
  if (nr == -1) {
    FILES[FILE_ANZ].fname = nocopy;
  } else {
    FILES[FILE_ANZ].fname = copyof;
  }
  FILES[FILE_ANZ].filesize = 0;

  FILES[FILE_ANZ].bootpic = 0;
  FILES[FILE_ANZ].blockalign = 0;
  FILES[FILE_ANZ].skip_bank = 0;

  FILES[FILE_ANZ].entrymode = mode;
  FILES[FILE_ANZ].newdiroffset = offset;
  FILES[FILE_ANZ].memory = 0;
  FILES[FILE_ANZ].pointer = 0;
  FILES[FILE_ANZ].copyof = nr;
  FILES[FILE_ANZ].inromsize = 0;
  FILES[FILE_ANZ].dirpointer = 0;
  FILES[FILE_ANZ].loadadr = 0;
  FILES[FILE_ANZ].addfileoffset = 0;

  FILE_ANZ++;
  return (true);
}

bool lynxrom::savelyx(char* fn)
{
  if (!writelyx) return (true);
  if (!data) return false;
  FILE* fh;

  //if(verbose)
  printf("Write LYX image with Blocksize %d Blocklimit %d CardLen %d MaxSize %d Audin %d Bank2 %d\n", blocksize, blockcount,  nCartLen, nMaxSize, (int)audin, (int)bank2);
  char* c;
  c = new char[strlen(fn) + 20];

  int banksize = blocksize * 256;
  int offset = 0;

  if (audin) {
    strcpy(c, fn);
    strcat(c, "_aud0_bank0.lyx");
    if(verbose) printf("Writing to %s\n", c);
    fh = fopen(c, "wb+");
    if (fh == 0) return (false);
    if (fwrite(data + offset, 1, banksize, fh) != banksize) printf("Error: Couldn't write %s !\n", fn);
    offset += banksize;
    fclose(fh);

    if (bank2) {
      strcpy(c, fn);
      strcat(c, "_aud0_bank1.lyx");
      if(verbose) printf("Writing to %s\n", c);
      fh = fopen(c, "wb+");
      if (fh == 0) return (false);
      if (fwrite(data + offset, 1, banksize, fh) != banksize) printf("Error: Couldn't write %s !\n", fn);
      offset += banksize;
      fclose(fh);
    }

    strcpy(c, fn);
    strcat(c, "_aud1_bank0.lyx");
    if(verbose) printf("Writing to %s\n", c);
    fh = fopen(c, "wb+");
    if (fh == 0) return (false);
    if (fwrite(data + offset, 1, banksize, fh) != banksize) printf("Error: Couldn't write %s !\n", fn);
    offset += banksize;
    fclose(fh);

    if (bank2) {
      strcpy(c, fn);
      strcat(c, "_aud1_bank1.lyx");
      if(verbose) printf("Writing to %s\n", c);
      fh = fopen(c, "wb+");
      if (fh == 0) return (false);
      if (fwrite(data + offset, 1, banksize, fh) != banksize) printf("Error: Couldn't write %s !\n", fn);
      offset += banksize;
      fclose(fh);
    }
  } else {
    if (bank2) {
      strcpy(c, fn);
      strcat(c, "_bank0.lyx");
      if(verbose) printf("Writing to %s\n", c);
      fh = fopen(c, "wb+");
      if (fh == 0) return (false);
      if (fwrite(data + offset, 1, banksize, fh) != banksize) printf("Error: Couldn't write %s !\n", fn);
      offset += banksize;
      fclose(fh);

      strcpy(c, fn);
      strcat(c, "_bank1.lyx");
      if(verbose) printf("Writing to %s\n", c);
      fh = fopen(c, "wb+");
      if (fh == 0) return (false);
      if (fwrite(data + offset, 1, banksize, fh) != banksize) printf("Error: Couldn't write %s !\n", fn);
      offset += banksize;
      fclose(fh);
    } else {
      if(verbose) printf("Writing to %s\n", fn);
      fh = fopen(fn, "wb+");
      if (fh == 0) return (false);

      if (filler) nCartLen = nMaxSize;
      if (fwrite(data, 1, nCartLen, fh) != nCartLen) printf("Error: Couldn't write %s !\n", fn);
      fclose(fh);
    }
  }

  delete c;
  return (true);
}

bool lynxrom::savelnx(char* fn)
{
  if (!writelnx) return (true);
  if (!data) return false;

  FILE* fh;
  struct LNX_STRUCT* ll;

  if(verbose) printf("Writing to %s\n", fn);
  fh = fopen(fn, "wb+");
  if (fh == 0) return (false);

  ll = new struct LNX_STRUCT;

  memset(ll, 0, sizeof(struct LNX_STRUCT));
  strncpy((char*)ll->magic, "LYNX",4);
  ll->page_size_bank0 = blocksize;
  if (bank2) {
    ll->page_size_bank1 = blocksize;
  }
  ll->version = 1;
  char* bn;
  bn = strrchr(fn, '/');
  if (bn == 0) bn = fn;
  bn = strrchr(fn, '\\');
  if (bn == 0) bn = fn;
  strncpy((char*)ll->cartname, lnxname, 32);
  ((char*)ll->cartname)[31] = 0;
  strncpy((char*)ll->manufname, lnxmanu, 16);
  ((char*)ll->manufname)[15] = 0;
  ll->rotation = lnxrot;
  ll->aud_bits = 0;
  if (audin) ll->aud_bits = 0x01;
  if (eeprom_type) ll->eeprom_bits = eeprom_type | (eeprom_8bit ? EEPROM_8BIT : EEPROM_16BIT);

  if (fwrite(ll, 1, sizeof(struct LNX_STRUCT), fh) != sizeof(struct LNX_STRUCT)) printf("Error: Couldn't write LN header for  %s !\n",
        fn);
  nCartLen = nMaxSize;

  //if(verbose) 
  printf("Write LNX with Blocksize %d Blocklimit %d CardLen %d MaxSize %d Audin %d Bank2 %d\n", blocksize, blockcount,  nCartLen, nMaxSize, (int)audin, (int)bank2);
  if (fwrite(data, 1, nCartLen, fh) != nCartLen) printf("Error: Couldn't write bank %s !\n", fn);

  fclose(fh);

  delete ll;
  return (true);
}

void lynxrom::copy_bll_header(void)
{
  if(!data) return;
  // Header kopieren
  if(verbose) printf("Copy internal BLL newloader.\n");
  for (int i = 0; i < 0x380; i++) data[i] = CardHead[i]; // 0x380 von 0x0408
  /*if( !checksum)*/{
    if(verbose) printf("Checksum disabled.\n");
    data[0x303] = 0x80; // Patch: BRA instead of BEQ --> No checksum required
    // remark by BS: Totally wrong, this is crctab-Adress!!!
    // very strange that this works!!!
  }
}

void lynxrom::copy_micro_header(void)
{
  if(!data) return;
  // Header kopieren
  if(verbose) printf("Copy internal micro loader.\n");
  switch (loader) {
    case L_MINI_F000: { // at $F000
      if(verbose) printf("... @ $F000\n");
      for (int i = 0; i < 52; i++) data[i] = micro_loader_f000_stage1[i];
      for (int i = 0; i < 151; i++) data[i + 52] = micro_loader_f000_stage2[i];
      // now comes the first dir entry in EPYX format! @ 203
      //    data[52+128+12+11+0]=FILES[0]  block nr
    }; break;
    case L_MINI_FB68: { // at $FB68
      if(verbose) printf("... @ $FB68\n");
      for (int i = 0; i < 52; i++) data[i] = micro_loader_fb68_stage1[i];
      for (int i = 0; i < 151; i++) data[i + 52] = micro_loader_fb68_stage2[i];
      // now comes the first dir entry in EPYX format! @ 203
      //    data[52+128+12+11+0]=FILES[0]  block nr
    }; break;
//     case 3:{// LEX at $F000
//       printf("... LEX @ $F000\n");
//       for (int i=0; i< 52; i++) data[i] = micro_loader_f000_stage1[i];
//       for (int i=0; i<206; i++) data[i+52] = menu_lex[i];
//       data[52+2]=FILES[0].dirpointer &0xFF;//dirofflo
//       data[52+3]=FILES[0].dirpointer >>8;//diroffhi
//       data[52+4]=(unsigned char)(0x100-(blocksize>>8));
//       data[52+5]=FILES[0].entrymode;// Epyxflag
//       data[52+7]=0*8;// Startfile
//     }; break;
    default:
      printf("... Type not supported!\n");
  }

  switch (loader) {
    case L_MINI_F000:
    case L_MINI_FB68:
      switch (blocksize) {
        case 512:
          data[52 + 128 + 12 + 11 - 7] = 0xfe; // 128kb
          break;
        case 1024:
          data[52 + 128 + 12 + 11 - 7] = 0xfc; // 256kb
          break;
        case 2048:
          data[52 + 128 + 12 + 11 - 7] = 0xf8; // 512kb
          break;
        default:
          printf("This Blocksize Is Not Supported By Microloader!!!\n");
      }
      break;
  }
}

bool lynxrom::process_files(void)
{
  int ndirold;
  if(verbose) printf("\n--- Process %d Files ---\n\n", FILE_ANZ);
  ndirold = oDirectoryPointer;
  for (int i = 0; i < FILE_ANZ; i++) {
    if (verbose>1) {
      printf("------ %d. entry in list ----------------------------\n", i);
    }
    ProcessFile(FILES[i]);
  }

  if(verbose) printf("\n");
  if (oCardTroyan > 0) {
    if (oDirectoryPointer < oCardTroyan + 8) { // Troyan Horse!!!
      if(verbose) printf("Before Troyan %d = %xh -> adjust Offset\n", oDirectoryPointer, oDirectoryPointer);
      oDirectoryPointer = oCardTroyan + 8;
    }
  }

  if(verbose) printf("Data start at Offset: %d = %xh\n", oDirectoryPointer, oDirectoryPointer);
  nCartLen = oDirectoryPointer;

  short nBlockNo;
  short nBlockOffset;
  nBlockOffset = nCartLen % blocksize;
  nBlockNo = nCartLen / blocksize;

  oDirectoryPointer = ndirold;
  if(verbose) printf("--- Directory processed ---\n\n");


  if (verbose) {
    printf("*********************************************\n");
    printf("nCartLen: %d = %xh\n", nCartLen, nCartLen);
    printf("nDirOffset: %d = %xh\n", oDirectoryPointer, oDirectoryPointer);
    printf("Block %xh, Offset %xh\n", nBlockNo, nBlockOffset);
    printf("*********************************************\n");
  }
  return true;
}

void lynxrom::ProcessFile(struct FILE_PAR& file)
{
  if(verbose) printf("nDirOffset: %d = %xh\n", oDirectoryPointer, oDirectoryPointer);
  // Jetzt die Datei
  if (oDirectoryPointer == oCardTroyan) {
    oDirectoryPointer += 8;// Troyan Horse!!!
    if(verbose) printf("Troyan Horse here!\n------- next entry ---\n");
  }

  if (file.newdiroffset > 0) {
    if (file.newdiroffset < oDirectoryPointer) {
      printf("Tried to set negative offset... new:%d < old:%d\n", file.newdiroffset, oDirectoryPointer);
    } else {
      if(verbose) printf("Set new Directory position offset... (old:%d)\n", oDirectoryPointer);
      oDirectoryPointer = file.newdiroffset;
      if(verbose) printf("nDirOffset: %d = %xh\n", oDirectoryPointer, oDirectoryPointer);
    }
  }

  if (file.fname == 0) {
    if (!file.bootpic) {
      printf("\n empty filename -> bug!\n");
      exit(100);
    }
    if(verbose) printf("Title Pic (built-in)\n");
  } else {
    if(verbose) printf("%s\n", file.fname);
  }

  if (file.copyof >= 0) {
    if(verbose) printf("COPY of entry %d\n", file.copyof);
  } else if (file.copyof == -1) {
    if(verbose) printf("Empty entry\n");
  }
  file.dirpointer = oDirectoryPointer;
  oDirectoryPointer += 8;
}

bool lynxrom::add_files(void)
{
  int tt = 0;
  struct FILE_PAR* file;

  if(verbose) printf("%d Files\n", FILE_ANZ);
  for (int i = 0; i < FILE_ANZ; i++) {
    short nBlockNo;
    short nBlockOffset;
    if (verbose) {
      printf("------ %d. entry in list ----------------------------\n", i);
    }
    file = &FILES[i];

    nBlockOffset = nCartLen % blocksize;
    nBlockNo = nCartLen / blocksize;

    if (file->blockalign) {
      // Dann Block-Align durchf?hren
      nCartLen += blocksize - 1;
      nCartLen &= ~(blocksize - 1);
      nBlockOffset = nCartLen % blocksize;
      nBlockNo = nCartLen / blocksize;
      if(verbose) printf("#ALIGN:\n");
      if (verbose) {
        printf("Next one will be at: Block %x, Offset %x\n", nBlockNo, nBlockOffset);
      }
    }

    if (file->fname == 0) {
      if (!file->bootpic) {
        printf("\n empty filename -> bug!\n");
        continue;
      }

      file->memory = dummy_insert;
      file->filesize = DUMMY_INSERT_SIZE;
      file->loadadr = 0x2400; // fixed adress for title pic

      if(verbose) printf("%d: Title Pic (built-in)\n", tt++);
    } else {
      if(verbose) printf("%d: %s\n", tt++, file->fname);
    }

    if (file->copyof >= 0) {
      if(verbose) printf("COPY of entry %d\n", file->copyof);
      continue;
    } else if (file->copyof == -1) {
      if(verbose) printf("Empty entry\n");
      continue;
    }
    file->inromloadoffset = nCartLen;
    if (file->memory == 0) {
      if (LoadFile(file) == 0) {
        printf("Bug: File %s could not be loaded!\n", file->fname);
        exit(100);
      }
    }
    AnalyseFile(file);// And write to Header
    if (WriteFileToRom(file)) {
      if (verbose) printf(" size: %d (%x) bytes", file->inromsize, file->inromsize);
      nCartLen += file->inromsize;

      if(verbose) printf("\n");
      if (verbose) {
        nBlockOffset = nCartLen % blocksize;
        nBlockNo = nCartLen / blocksize;
        if(verbose) printf("File handled:\n");
        if(verbose) printf("Next one will be at:  %x, Offset %x\n", nBlockNo, nBlockOffset);
      }

    } else {
      printf("File NOT handled:\n");
      break;
    }
  }

  if (verbose) {
    short nBlockOffset;
    short nBlockNo;
    nBlockOffset = nCartLen % blocksize;
    nBlockNo = nCartLen / blocksize;
    printf("*********************************************\n");
    printf("nCartLen: %d = %xh\n", nCartLen, nCartLen);
    printf("nDirOffset: %d = %xh\n", oDirectoryPointer, oDirectoryPointer);
    printf("Block %xh, Offset %xh\n", nBlockNo, nBlockOffset);
    printf("*********************************************\n");
  }

  if (!is_minihead() && FILES[0].loadadr == 0) {
    printf("Set Title Adress to %d ($%04X) as none was defined within title file header.\n", titleadr, titleadr);
    FILES[0].loadadr = titleadr; // fixed adress for title pic
  }

  for (int i = 0; i < FILE_ANZ; i++) {
    WriteDirEntry(&FILES[i]);
  }
  return true;
}

bool lynxrom::WriteFileToRom(struct FILE_PAR* file)
{
  if(!data) return false;
  if (nCartLen + file->inromsize > nMaxSize) {
    printf("\nROM Size exceeded!\n");
    return (false);
  }

  int bank1offset = -1, bank2offset = -1, audinoffset = -1;
  if (audin) {
    if (bank2) {
      bank1offset = 1 * nMaxSize / 4;
      audinoffset = 2 * nMaxSize / 4;
      bank2offset = 3 * nMaxSize / 4;
    } else {
      audinoffset = nMaxSize / 2;
    }
  } else {
    if (bank2) {
      bank1offset = nMaxSize / 2;
    } else {
      // do nothing
    }
  }

  if(contcont){
      if (bank1offset > 0 && nCartLen <= bank1offset && (file->skip_bank || nCartLen + file->inromsize > bank1offset)) {
        printf("\nBank full, wrap ($%X to $%X of $%X)\nNO Correction -- does not work for AUDIN!\n", nCartLen, nCartLen + file->inromsize, bank1offset);
      }
  }else{
    if (bank1offset > 0 && nCartLen <= bank1offset && (file->skip_bank || nCartLen + file->inromsize > bank1offset)) {
        printf("\nB1: Bank full ($%X of $%X) -> next ($%X)\n", nCartLen + file->inromsize, bank1offset, bank1offset);
        nCartLen = bank1offset;
        file->inromloadoffset = nCartLen;
        printf("Correction:  $%x, Offset %x\n", nCartLen / blocksize, nCartLen % blocksize);
    } else if (audinoffset > 0 && nCartLen <= audinoffset && (file->skip_bank || nCartLen + file->inromsize > audinoffset)) {
        printf("\nAU: Bank full ($%X of $%X) -> next ($%X)\n", nCartLen + file->inromsize, audinoffset, audinoffset + blocksize);
        nCartLen = audinoffset + AUDIN_OFFSET;
        file->inromloadoffset = nCartLen;
        printf("Correction:  $%x, Offset $%x\n", nCartLen / blocksize, nCartLen % blocksize);
    } else if (bank2offset > 0 && nCartLen <= bank2offset && (file->skip_bank || nCartLen + file->inromsize > bank2offset)) {
        printf("\nB2: Bank full ($%X of $%X) -> next ($%X)\n", nCartLen + file->inromsize, bank2offset, bank2offset);
        nCartLen = bank2offset;
        file->inromloadoffset = nCartLen;
        printf("Correction:  $%x Blocks, Offset $%x Bytes\n", nCartLen / blocksize, nCartLen % blocksize);
    }
  }

  memcpy(data + nCartLen, file->pointer, file->inromsize);
  if (file->addfileoffset) {
    nCartLen += file->addfileoffset;
    printf("Skip %d bytes after", file->addfileoffset);
  }
  return (true);
}

bool lynxrom::WriteDirEntry(struct FILE_PAR* file)
{
  unsigned char* pDirectoryPointer;
  int bo, bn, fl;
  if(!data) return false;

  if (file->copyof >= 0) {
    WriteDirCopyEntry(&FILES[file->copyof], file->dirpointer, file->entrymode);
    return true;
  } else if (file->copyof == -1) {
    WriteDirZeroEntry(file->dirpointer);
    return true;
  }
  pDirectoryPointer = data + file->dirpointer;

  bo = file->inromloadoffset % blocksize;
  bn = file->inromloadoffset / blocksize;
  fl = file->indirfilesize;

  if (bank2) { // bank2
    if (bn & 0x100) bo |= 0x4000;
    if (audin) { // and audin
      if (bn & 0x200) bo |= 0x8000;
    }
  } else {
    if (audin) { // only audin
      if (bn & 0x100) bo |= 0x8000;
    }
  }

  pDirectoryPointer[0] = bn;
  pDirectoryPointer[3] = file->flag;
  pDirectoryPointer[4] = file->loadadr & 0xff;  // Steht "falsch" rum drin
  pDirectoryPointer[5] = file->loadadr >> 8;
  if (file->entrymode) { //EPYX
    pDirectoryPointer[1] = ((bo) & 0xff);
    pDirectoryPointer[2] = ((bo) >> 8);
    pDirectoryPointer[6] = ((fl) & 0xff);
    pDirectoryPointer[7] = ((fl) >> 8);
  } else { // BLL
    pDirectoryPointer[1] = ((bo ^ 0xffff) & 0xff);
    pDirectoryPointer[2] = ((bo ^ 0xffff) >> 8);
    pDirectoryPointer[6] = ((fl ^ 0xffff) & 0xff);
    pDirectoryPointer[7] = ((fl ^ 0xffff) >> 8);
  }
  return true;
}

bool lynxrom::WriteDirCopyEntry(struct FILE_PAR* file, int dirpointer, bool mode)
{
  unsigned char* pDirectoryPointer;
  int bo, bn, fl;
  if(!data) return false;

  pDirectoryPointer = data + dirpointer;

  bo = file->inromloadoffset % blocksize;
  bn = file->inromloadoffset / blocksize;
  fl = file->indirfilesize;

  if (bank2) { // bank2
    if (bn & 0x100) bo |= 0x4000;
    if (audin) { // and audin
      if (bn & 0x200) bo |= 0x8000;
    }
  } else {
    if (audin) { // only audin
      if (bn & 0x100) bo |= 0x8000;
    }
  }

  pDirectoryPointer[0] = bn;
  pDirectoryPointer[3] = file->flag;
  pDirectoryPointer[4] = file->loadadr & 0xff;  // Steht "falsch" rum drin
  pDirectoryPointer[5] = file->loadadr >> 8;
  if (mode) { //EPYX
    pDirectoryPointer[1] = ((bo) & 0xff);
    pDirectoryPointer[2] = ((bo) >> 8);
    pDirectoryPointer[6] = ((fl) & 0xff);
    pDirectoryPointer[7] = ((fl) >> 8);
  } else { // BLL
    pDirectoryPointer[1] = ((bo ^ 0xffff) & 0xff);
    pDirectoryPointer[2] = ((bo ^ 0xffff) >> 8);
    pDirectoryPointer[6] = ((fl ^ 0xffff) & 0xff);
    pDirectoryPointer[7] = ((fl ^ 0xffff) >> 8);
  }
  return true;
}

bool lynxrom::WriteDirZeroEntry(int dirpointer)
{
  unsigned char* pDirectoryPointer;
  if(!data) return false;
  pDirectoryPointer = data + dirpointer;
  for (int i = 0; i < 8; i++) pDirectoryPointer[i] = 0;
  return true;
}

bool lynxrom::AnalyseFile(struct FILE_PAR* file)
{
  int nTmpFileLength;
  unsigned char fflag;

  file->pointer = file->memory;
  file->data = file->memory;
  file->inromsize = file->filesize;
  file->indirfilesize = file->inromsize;

  if (strncmp((const char*)file->memory, "ROMRIP", 6) == 0) {
    file->pointer += 16;
    file->inromsize -= 16;
    file->indirfilesize -= 16;
    file->data += 16;
    file->flag = ((unsigned char*)file->memory)[8 + 2];
    file->loadadr = ((unsigned short*)file->memory)[8 / 2];
    if(verbose) printf("RomRip ");
  } else if(file->memory[0]==0xFF && file->memory[1]==0xFF){
      printf("hmmm could be COM header %02X %02X %02X %02X\n", file->memory[2], file->memory[3], file->memory[4], file->memory[5]);
      file->pointer += 6;
      file->inromsize -= 6;
      file->flag = 0;
      file->loadadr = file->memory[3] * 256 + file->memory[2]; // Startadresse
      // keine ahnugn  was der rest ist...
  } else {
    fflag = file->memory[0] | file->memory[1];
//    if ((fflag == 0x88) ||  // Normales Programm
//      (fflag == 0x89))  // Gepacktes Programm
    if (file->memory[1] == 0x80 && (file->memory[0] == 0x08 || file->memory[0] == 0x09)) {
      printf("hmmm could be wrong Endian header %02X %02X\n", file->memory[0], file->memory[1]);
    }
    /// Problem: Atari vs PC /
    if (file->memory[0] == 0x80 && (file->memory[1] == 0x08 || file->memory[1] == 0x09)) {
      // Korrigierten Offset
      if (skipheader) {
        file->pointer += HEADERLEN;
        file->inromsize -= HEADERLEN;
      } else {
        file->inromloadoffset += HEADERLEN;
      }
      file->indirfilesize -= HEADERLEN;
      file->data += HEADERLEN;


      file->flag = fflag;
      file->loadadr = file->memory[2] * 256 + file->memory[3]; // Startadresse

      if (fflag == 0x89) { // Gepacktes Prg.
        file->indirfilesize = file->memory[4] * 256 + file->memory[5];
        // TP packed program ... length in dir is unpacked(!) length not packed length
        if(verbose) printf("TP Exec ");
      } else {
        if(verbose) printf("Exec ");
      }
    }

    // Check if we have IMP! File
    if (file->data[0] == 'I' && file->data[1] == 'M' && file->data[2] == 'P' && file->data[3] == '!') {
      file->flag = 'I';// Data imploded
      if(verbose) printf("Imploded ");
      if (delimp) { // ... and remove Magic
        file->data[0] = random() & 0xFF;
        file->data[1] = random() & 0xFF;
        file->data[2] = random() & 0xFF;
        file->data[3] = random() & 0xFF;
      }
    }
    // Check if we have PCrunch File
    if (file->data[2] == 'p' && file->data[3] == 'u') {
      file->flag = 'P';// Data pCrunched
      if(verbose) printf("PUCrunch ");
      file->loadadr = file->data[7] + file->data[8] * 256;
      if(verbose) printf("load to %04X ", file->loadadr);
      file->data[14] = file->data[12];
      file->data[13] = file->data[11];
      file->data[12] = file->data[10];
      file->data[11] = file->data[9];
      file->data[10] = file->data[6];
      file->pointer += 10;
      file->inromsize -= 10;
      file->indirfilesize -= 10;
      file->data += 10;
    }
  }
  // noPrg

  return (true);
}

bool lynxrom::built(void)
{
  if(loader==L_UNDEF){
    printf("No Loader selected. If you want to create a ROM without loader, please state #NOLOADER\n");
    exit(1000);
  }
  if(!data){
    printf("something is fishy, pleae check your mak file.\n");
    exit(1000);
  }
  AUDIN_OFFSET=blocksize;
  if (is_minihead()) AUDIN_OFFSET=AUDIN_OFFSET_MICRO;// tested und O.k.
  if (is_hackhead()) AUDIN_OFFSET=AUDIN_OFFSET_HACK;// ob das o.k. ist?
  if (loader==L_NOLOADER) memset(data, oDirectoryPointer, 0); // clean up with zeros (for encryption)
  if (is_internal_bll()) copy_bll_header();

  process_files();
  add_files();

  if (is_minihead()) copy_micro_header();
  if (oCardTroyan > 0) {
    if(verbose) printf("Now write troyan entry\n");
    unsigned char troy[] = {0, 0, 2, 0, 0, 2, 0, 2};
    memcpy(data + oCardTroyan, troy, 8);
  }
  if(verbose) printf("==> Size: %d %x \n", nCartLen, nCartLen);

  if( loader==L_HACKAUTO){
    switch(blocksize){
      case 512:
        loader=L_HACK512;
        break;
      case 1024:
        loader=L_HACK1024;
        break;
      case 2048:
        loader=L_HACK2048;
        break;
      default:
        printf("=== HACK AUTO: Block size not supported!!!\n");
        break;
    }
  }
  if (is_hackhead()) {
    if(verbose) printf("=== Set Hacked encrypted header\n");
    switch (loader) {
      case L_HACK512:
        if(verbose) printf("=== type 512\n");
        memcpy(data, hackload_stage1, 154);
        memcpy(data + 154, hackload_stage2_512, 256);
        break;
      case L_HACK1024:
        if(verbose) printf("=== type 1024\n");
        memcpy(data, hackload_stage1, 154);
        memcpy(data + 154, hackload_stage2_1024, 256);
        break;
      case L_HACK2048:
        if(verbose) printf("=== Type 2048\n");
        memcpy(data, hackload_stage1, 154);
        memcpy(data + 154, hackload_stage2_2048, 256);
        break;
      default:
        printf("=== Block size not supported!!!\n");
        break;
    }
  }
  if (audin) {
    memcpy(data + nMaxSize / 2, data, AUDIN_OFFSET); // maybe 256 bytes is enough >> to test on hardware!
    if(verbose) printf("Duplicate Header for AUDIN 1 (%d bytes)\n", AUDIN_OFFSET);
  }
  return true;
}

void lynxrom::SetLoader(int t)
{
    loader = t;
}

void lynxrom::SetHackHeader(int t,int n)
{
    SetLoader(t);
    SetBlockSize(n);
    SetDirStart(410);
}

void lynxrom::SetInternalLoader(void)
{
    SetLoader(L_BLL);
    SetBlockSize(1024);
    SetDirStart(0x380);
}

void lynxrom::SetMiniHeader(int t)
{
    SetLoader(t);
    SetDirStart(203);
    if(!blocksize) SetBlockSize(512);
}
