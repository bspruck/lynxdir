;
; Example for a complex layout
; miniloader, banks, audin switching
; this gives a 2 MByte cartridge
;
#LNXNAME SlideShow
#LNXMANU lynxdir
#LNXROT 0
#NEWMINI_FB68
#BLOCKSIZE 2048
#BANK2
#AUDIN
#EPYX
#DIRSTART 203
unpacker.o
#BLL
maincode.o
music.o
;
file1
file2
file3
; and more and more and more
; banks will be filles automatically... but we can force it too
#SKIP_BANK
file27
file28
;...
file200
; maximum 256 files ALLTOGETHER
; EOF
