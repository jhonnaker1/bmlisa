#
# idle.mk - the IDLE and Musashi sources BMLisa builds.
#
# IDLE's come from third_party/idle, its Subversion repository
# (svn.code.sf.net/p/idle-lisa-emu/code) at revision 26, unmodified but for
# files whose licences don't allow BMLisa, which aren't there (see
# docs/THIRD_PARTY.md): its 68000, Musashi 3.3 (cpu/m68k*), and its serial
# chip (io/8530scc.c). They are the files its Makefile.unixsdl compiles,
# minus those and the SDL front end (main_sdl.cpp, sdlgui.cpp,
# dlgFileSelect.cpp, file.cpp, scandir.c): src/port/lisa.cpp does what
# main_sdl.cpp did. IDLE compiles them all as C++.
#
# In their place:
#  - the 68000 is Musashi 4 (third_party/musashi, MIT licence), unmodified,
#    built with src/port/bmlisa_m68kconf.h and src/port/lisa_cpu.c;
#    src/port/idle/cpu/ has the headers IDLE's files include from cpu/
#    (see src/port/idle/README.md);
#  - the serial chip is src/port/scc.cpp.
#
# IDLE's files are compiled with src/port/idle_port.h included first
# (-include), which sends IDLE's fopen() and exit() to BMLisa (see there).
#

IDLE_SRCS = \
	cpu/debug.c \
	debug/trace.c \
	disk/disk_ctrl.c \
	disk/disk_image.c \
	io/via.c \
	io/via_hdd.c \
	io/via_keyb.c \
	irq/irq.c \
	mmu/mmu.c \
	time/timer.c

IDLE_CFLAGS = -x c++ -w

# Musashi: m68kmake (built for the machine doing the build) writes
# m68kops.c and m68kops.h from m68k_in.c. m68kcpu.c includes m68kfpu.c,
# which calls SoftFloat; SoftFloat (softfloat/softfloat.c) isn't built (its
# licence isn't the GPL's), src/port/lisa_nofpu.c stands in for it.
MUSASHI_SRCS = m68kcpu.c
MUSASHI_GEN_SRCS = m68kops.c
MUSASHI_CFLAGS = -w -DMUSASHI_CNF='"bmlisa_m68kconf.h"'
