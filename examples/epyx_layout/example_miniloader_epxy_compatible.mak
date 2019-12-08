;
; Example file
; Use MINI loader, but the (new) EPYX directory layout
; Title picture will be skipped!
;
#NEWMINI_FB68
#BLOCKSIZE 1024
#DIRSTART 203
#EPYX

;; Now the files
main.o
;; Advance to make the layout EPXY compatible
#DIROFFSET 410
;; The new miniloader is using NO title picture loader! but to keep offsets... put dummy here... it will not be shown
#PUTTITLE
;; Now put a copy of the first dir entry (main.o)
#COPY 0
