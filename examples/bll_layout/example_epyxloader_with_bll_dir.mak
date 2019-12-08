;
; Example file, BLL compatible (newload)
; but using the EPYX encryption, NOT the troyan
; for compatibility, file 16 is the troyan, still
;
#TROYAN
#HACK1024
#BLOCKSIZE 1024
#DIRSTART 410
#EPYX
title.o
main.o
#BLL
#DIROFFSET 896
;; now copy the first two dir entries
#COPY0
#COPY1
; data 1
; data 2
; etc
