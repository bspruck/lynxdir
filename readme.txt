
*********************************
*          LYNXDIR              *
*         Version 1.9           *
*   (c) Björn Spruck 2010-2017  *
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

HACK512, HACK1024, HACK2048, HACKAUTO - Put the Epxy hacked loader in place. Adjusts the BLOCKSIZE accordingly. Sets DIRSTART to 203. Mode to EPXY. AUTO will set loader depending on selected blocksize.
NEWMINI_F000, NEWMINI_FB68 - Put the new mini header plus loader into place, set DIRSTART to be 203, blocksize is variable, default 512. Mode to EPXY.
INTERNAL - Put the BLL hacked loader into place, this should be followed by an "TROYAN" command. Sets the blocksize to 1024 and the DIRSTART to 896. Mode to BLL.
NOLOADER - dont put any loader code, just zeros. need to run encryption script afterwards. Makes only sense for EPYX layouts.

If there is no loader defined (or NOLOADER given) the program will exit.

TROYAN - Put the Toryan horse into place, only needed for BLL compatible layouts.

BLOCKSIZE and DIRSTART are now set by commands above as well as the mode for the first dir entry.
BLOCKSIZE - Blocksize for this ROM, one of the most important comnmands. Possible values are: 512 (default), 1024, 2048
BLOCKLIMIT - limits the filling of unused blocks to a given number in case the ROM is smaller. default 256 (does nothing)
DIRSTART - where to start the directory, 410 for EPYX loader, 896 for BLL, 203 for NewMini
AUDIN - doubles the number of banks by using AUDIN line for switching (the user code is responsible for supporting this)
BANK2 - doubles the number of banks by using second read strobe (the user code is responsible for supporting this)

The usage of AUDIN and BANK2 requires additional bits in the lnx header to be set. Only the newest versions of handy emulator code support it (version 0.98).
The order in which the banks are written to the file is discussed by length in atariage -> lynx -> development forum.
The users code is responsible for switching banks and reading files correctly. There is no official support.

TITLEADR - specify title loading adress, if not specifies within title file. If a .o file is used, dont give it. Remark: This address MUST fit the one which was used when the title file was created.
PUTTITLE - Put the internal "Insert Name" title picture into place. Only allowed as a replacement for the first file (but NOT for mini loaders!)

DIROFFSET - Moves directory pointer to a different (higher) offset. Needed for BLL/EPYX/MINI mixed loader
FILEADR - Sets the default loading adress for files (default 0000) if not defined within file header.
ALIGN - Align next file to a block offset of zero in ROM
EPYX - Switch to EPYX style directory entries.
BLL - Switch to BLL style directory entries (XORing some entries).
COPY - Adds a directory entry which is a copy of another one added before, without duplicating the file data. Omitting the number makes a copy of the previous entry. Note: if the mode was switched in between, the entry will be adapted accordingly.
NONE - write an empty directory entry
EMPTY - same as NONE
SKIP - skip n entries in directory (= write n empty entries)
SKIP_BANK - skips to the next bank (if available).

INCLUDE - includes another mak file with given file name (for complex projects)

And a few handy things.

NOLYX - Dont write a plain ROM image, LYX
NOLNX - Dont write a emulator style ROM image, LNX
LNXNAME - Put this cartname in LNX header (31+1)
LNXMANU - Put this manufacturer in LNX header (15+1)
LNXROT - Set rotation flag for emulation (0,1,2 - NONE, LEFT, RIGHT)

Set EEPROM flags (for emulation)

EE_93C46, EE_93C56, EE_93C66, EE_93C76, EE_93C86 - set bits to inform emulator about the presence of an SPI eeprom of this type
EE_16BIT, EE_8BIT - set the access type to 8/16 bit mode. 16 bit mode is default.
