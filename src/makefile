# makefile for ffnes project
# written by rockcarry

# for pthreads-win32
PTHREADSW32_DLL = ../pthreads-win32/dll/pthreadGC2.dll
PTHREADSW32_INC = ../pthreads-win32/include
PTHREADSW32_LIB = ../pthreads-win32/lib

# for ffencoderlib
FFENCODERLIB_DLL = ../ffencoder/ffencoder.dll
FFENCODERLIB_INC = ../ffencoder
FFENCODERLIB_LIB = ../ffencoder

# 编译器定义
CC      = i586-mingw32msvc-gcc
CCFLAGS = -mwindows -Wall -I$(PTHREADSW32_INC) -I$(FFENCODERLIB_INC)

# 所有的目标文件
OBJS = \
    log.o \
    mem.o \
    bus.o \
    nes.o \
    cpu.o \
    ppu.o \
    apu.o \
    mmc.o \
    ndb.o \
    lzw.o \
    cartridge.o \
    joypad.o \
    replay.o \
    adev.o \
    vdev-gdi.o \
    vdev-d3d.o \

# 所有的可执行目标
EXES = \
    emulator.exe \

# 输出的库文件
DLL = ffnes.dll

# pthread dll
PTHREADDLL = pthreadGC2.dll
FFENCODERDLL = ffencoder.dll

# 编译规则
all : $(DLL) $(EXES) $(PTHREADDLL) $(FFENCODERDLL)

$(DLL) : $(OBJS)
	$(CC) $(CCFLAGS) -o $@ $(OBJS) --share -lwinmm -ld3d9 $(PTHREADSW32_DLL) $(FFENCODERLIB_DLL)

%.o : %.c %.h stdefine.h
	$(CC) $(CCFLAGS) -o $@ $< -c

%.o : %.cpp %.h stdefine.h
	$(CC) $(CCFLAGS) -o $@ $< -c

%.o : %.c stdefine.h
	$(CC) $(CCFLAGS) -o $@ $< -c

%.o : %.cpp stdefine.h
	$(CC) $(CCFLAGS) -o $@ $< -c

%.exe : %.c %.h $(DLL)
	$(CC) $(CCFLAGS) -o $@ $< $(DLL)

%.exe : %.cpp %.h $(DLL)
	$(CC) $(CCFLAGS) -o $@ $< $(DLL))

$(PTHREADDLL) : $(PTHREADSW32_DLL)
	cp $< .

$(FFENCODERDLL) : $(FFENCODERLIB_DLL)
	cp $< .

clean :
	-rm -f *.o
	-rm -f *.dll
	-rm -f *.exe

# rockcarry
# 2006.10.4
# THE END



