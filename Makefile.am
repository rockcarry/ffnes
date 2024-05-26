AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/src

libffnesincludedir = $(includedir)
libffnesinclude_HEADERS = src/*.h

lib_LTLIBRARIES = libffnes.la

libffnes_la_SOURCES = \
    src/mem.c src/bus.c src/cpu.c src/mmc.c src/nes.c src/cartridge.c src/joypad.c

libffnes_la_LDFLAGS = -no-undefined

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = src/libffnes.pc

if ENABLE_DEBUG
CFLAGS += -DDEBUG
endif

bin_PROGRAMS  = emulator
emulator_SOURCES = src/emulator.c
emulator_LDADD   = $(top_builddir)/libffnes.la
