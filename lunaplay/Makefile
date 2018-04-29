# TODO: comment

PROG= lunaplay
SRCS= \
	lunaplay.c \
	psgconv.c \
	psgpcm.c \
	psgvt.c \
	wav.c \
	devxp.c \
	filehelper.c \
	format.c

LDADD+= -lm

.PHONY:	gentbl
gentbl:
	${MAKE} -f Makefile.gentbl

devxp.c:	firmware.inc

cdump:	cdump.c

xppcm.inc:	cdump xppcm.rom
	./cdump xppcm.rom > xppcm.inc

.include <bsd.prog.mk>
