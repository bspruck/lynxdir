;
; Example file, creates BLL 1024bpb cart with loader
; troyan horse at file position 16
; "new" loader
;
#TROYAN
#INTERNAL
#BLOCKSIZE 1024
#DIRSTART 896
#BLL
;insert.o
;#TITLEADR 32000
#PUTTITLE
main.o
; data1.dat
; data2.dat
; etc
