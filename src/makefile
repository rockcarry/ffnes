# makefile for ffnes project
# written by rockcarry

# 编译器定义
CC      = i586-mingw32msvc-gcc
CCFLAGS = -mwindows -Wall

# 所有的目标文件
OBJS = \
    mem.o \
    bus.o \
    nes.o \

# 所有的可执行目标
EXES = \
    nes.exe \

# 输出的库文件
DLL = nes.dll

# 编译规则
all : $(DLL) $(EXES)

$(DLL) : $(OBJS)
	$(CC) $(CCFLAGS) -o $@ $(OBJS) --share -lddraw

%.o : %.c %.h stdefine.h
	$(CC) $(CCFLAGS) -o $@ $< -c

%.o : %.cpp %.h stdefine.h
	$(CC) $(CCFLAGS) -o $@ $< -c

%.o : %.c stdefine.h
	$(CC) $(CCFLAGS) -o $@ $< -c

%.o : %.cpp stdefine.h
	$(CC) $(CCFLAGS) -o $@ $< -c

%.exe : %.c %.h $(DLL)
	$(CC) $(CCFLAGS) -o $@ $< $(DLL) -D_TEST_

%.exe : %.cpp %.h $(DLL)
	$(CC) $(CCFLAGS) -o $@ $< $(DLL) -D_TEST_

clean :
	-rm -f *.o
	-rm -f *.dll
	-rm -f *.exe

# RockCarry
# 2006.10.4
# THE END



