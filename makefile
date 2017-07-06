# Piece 用 makefile

# 拡張子の定義

.SUFFIXES:
.SUFFIXES:  .o .s .c

# 生成コマンド・オプションのマクロ

CC = c33-pcc
#CC = pcc33

CFLAGS = -c -gp=0x0 -near -O2 -Wall

AS = c33-pcc
#AS = pcc33
ASFLAGS = -c -gp=0x0 -near
LD = c33-pcc
#LD = pcc33
LDFLAGS = -ls -lm

# 生成規則

.c.o:
	$(CC) $(CFLAGS) $<

.s.o:
	$(AS) $(ASFLAGS) $<

# 構成ファイル・生成ファイルのマクロ

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

# 依存関係

# フラッシュ書き込みイメージ生成
pex : $(PRGNAME).srf
	ppack -e $(PRGNAME).srf -o$(FILENAME).pex -n$(CAPTION) -i$(ICON)

# クリーンアップ
clean:
	del $(PRGNAME).sym
	del $(PRGNAME).map
	del *.o
	del $(PRGNAME).srf
	del $(FILENAME).pex
	
