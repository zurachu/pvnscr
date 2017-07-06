# Piece �p makefile

# �g���q�̒�`

.SUFFIXES:
.SUFFIXES:  .o .s .c

# �����R�}���h�E�I�v�V�����̃}�N��

CC = c33-pcc
#CC = pcc33

CFLAGS = -c -gp=0x0 -near -O2 -Wall

AS = c33-pcc
#AS = pcc33
ASFLAGS = -c -gp=0x0 -near
LD = c33-pcc
#LD = pcc33
LDFLAGS = -ls -lm

# �����K��

.c.o:
	$(CC) $(CFLAGS) $<

.s.o:
	$(AS) $(ASFLAGS) $<

# �\���t�@�C���E�����t�@�C���̃}�N��

PRGNAME = pvns
FILENAME = pvnscr
#FILENAME = pvnscr_m
CAPTION = P/VNScripter
ICON =	icon.pid
OBJS =	pvns.o\
		file.o\
		gamelib.o\
		sfont.o\
		htomei2.o\
		instdef2.o\
		arrow.o
LIBS =	libfpk\\libfpk.lib


$(PRGNAME).srf : $(OBJS)
	$(LD) $(LDFLAGS) -e$(PRGNAME).srf $(OBJS)  $(LIBS)

# �ˑ��֌W

# �t���b�V���������݃C���[�W����
pex : $(PRGNAME).srf
	ppack -e $(PRGNAME).srf -o$(FILENAME).pex -n$(CAPTION) -i$(ICON)

# �N���[���A�b�v
clean:
	del $(PRGNAME).sym
	del $(PRGNAME).map
	del *.o
	del $(PRGNAME).srf
	del $(FILENAME).pex
	
