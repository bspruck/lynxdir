;
; Example file, creates BLL 1024bpb cart with EPYX loader
; troyan horse at file position 16
; "new" BLL loader directory layout
;
#TROYAN
#INTERNAL
#BLOCKSIZE 1024
#DIRSTART 896
#BLL
;; put your own title picture, address is only necessary if binayr w/o header (e.g. no .o file) is used
;#TITLEADR 32000
;title.o
;; put the internal title picture
#PUTTITLE
main.o
; data1.dat
; data2.dat
; etc
