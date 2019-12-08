
*********************************
*          LYNXDIR              *
*         Version 1.7           *
*   (c) Björn Spruck 2010-2015  *
*********************************

What it does:
This program builts a ROM directory for the lynx according to
different "standards" and adds an encrypted loader to it.
The encrypted loader will show the title picture (first file) and
loads the second file for execution.
No checksumming is done and no directory entries are stored within
the loader, therefore the loader can be just copied and the content of
the ROM can be changed by hand if needed.
Loaders for 512/1024/2048 bytes per block are supported. Means 128k, 2565k and 512k ROMs.
The user can, if needed, lateron do a real encryption on the ROM image
with an external program.

Calling the program without arguments will show a list of possible parameters. Most of them are not
too useful and only for special cases (ROM encryption, compatibility, etc).
The behaviour of this program mainly steered with a config file "*.mak". In this file the content
of the ROM is listed, means, the filenames of the files which should go there in their
respective order. A lot of parameters can be set in the mak file, most of them you will
need only in very rare cases. Look at the examples.

The first two filenames have to be the title picture (in a special format not discussed here)
and the "main" executeable. If no title picture is given, the programs internal one can be used.
If you want to have a no picture (=black screen), put in a file with 40 zero bytes.

Compatibility with the old lynxer:
Well, some parameters changed their meaning.
The "simple mode" is creating now an EPYX style 128kb ROM (was BLL 256kb in lynxer).
A .lyx and .lnx file is created.


-----------------------
: Rules for mak files :
-----------------------

Comments start with a ";"
Command start with a "#"
everything else is a filename
empty lines are ignored
whitespaces (in most cases) are ignored
filenames have to be a) relative to directory or b) absolute.
filenames should not contain spaces or special characters (try it out)

Parameters can be divided in two groups:
1) Commands affecting the whole ROM, independend where they show up in the file.
2) Commands affecting only the next or all following files (switches). Here the position matters.
For readability case 1) should be at the beginning of the file.

The following parameters are supported in mak files:

BLOCKSIZE - Blocksize for this ROM, one of the most important comnmands. Possible values are: 512, 1024, 2048
HACK512, HACK1024, HACK2048 - Put the Epxy hacked loader in place; you do not have to use the same as specified by blocksize, but you should think about the consequences if not doing so ;-)
NEWMINI_F000, NEWMINI_FB68 - Put the new mini header plus loader into place, requires DIRSTART to be 203, blocksize is variable
INTERNAL - Put the BLL hacked loader into place, this should be followed by an "TROYAN" command.
TROYAN - Put the Toryan horse into place, only needed for BLL compatible layouts.
DIRSTART - where to start the directory, 410 for EPYX loader, 896 for BLL, 203 for NewMini

TITLEADR - specify title loading adress, if not specifies within title file
PUTTITLE - Put the internal "Insert Name" title picture into place. Only allowed as a replacement for the first file

DIROFFSET - Moves directory pointer to a different (higher) offset. Needed for BLL/EPYX mixed loader
FILEADR - Sets the default loading adress for files (default 0000) if not defined within file header.
ALIGN - Align next file to a block offset of zero in ROM
EPYX - Switch to EPYXL style directory entries.
BLL - Switch to BLL style directory entries.
COPY - Adds a directory entry which is a copy of another one added before, without duplicating the file data.
NONE - write an empty directory entry
EMPTY - same as NONE
SKIP - skip n entries in directory (= write n empty entries)

NOLYX - Dont write a plain ROM image, LYX
NOLNX - Dont write a emulator style ROM image, LNX
LNXNAME - Put this cartname in LNX header (31+1)
LNXMANU - Put this manufacturer in LNX header (15+1)
LNXROT - Set rotation flag
