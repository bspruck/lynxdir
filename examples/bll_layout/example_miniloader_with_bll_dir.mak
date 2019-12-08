;
; Example file, BLL compatible directory (newload)
; but using the MINI loader
; for compatibility, file 16 is the troyan, still
;
#NEWMINI_FB68
;; Blocksize is variable but BLL uses 1024
#BLOCKSIZE 1024
;; Dir start for miniloader
#DIRSTART 203
#EPYX
main.o
;; Advance to BLL directory position
#DIROFFSET 896
#BLL
;; The new miniloader is using NO title picture, nevertheless, we have to fill the dir entry (could be #EMPTY, too)
#PUTITLE
#COPY 0
