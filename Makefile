
.DEFAULT: lynxdir

lynxdir: lynxdir.cpp lynxrom.cpp lynxrom.h hack_l1.h hack_l2_1024.h hack_l2_2048.h
	g++ -o lynxdir lynxdir.cpp lynxrom.cpp
