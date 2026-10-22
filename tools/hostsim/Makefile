#
# Builds the PC test harness for the emulator port (see host_sim.c).
#
#   make -C tools/hostsim
#   tools/hostsim/run_tests.sh
#

ROOT := ../..
IDLE := $(ROOT)/third_party/idle
MUSASHI := $(ROOT)/third_party/musashi
include $(ROOT)/src/port/idle.mk

CC ?= cc
CXX ?= c++
OUT := build

CFLAGS = -O2 -g -std=gnu11 -I $(ROOT)/src/port -DBMLISA_HOST_LIBC
# Musashi, with BMLisa's settings and the opcode tables m68kmake wrote.
MUSASHI_FLAGS = -O2 -g -I $(OUT)/musashi -I $(MUSASHI) -I $(ROOT)/src/port $(MUSASHI_CFLAGS)
# IDLE's files, and BMLisa's that use its headers, as IDLE builds them;
# src/port/idle/cpu first, for "../cpu/m68k.h" (src/port/idle/README.md).
IDLE_FLAGS = -O2 -g -I $(ROOT)/src/port/idle/cpu -I $(ROOT)/src/port/idle -I $(IDLE) \
             -I $(ROOT)/src/port -I $(ROOT)/third_party \
             -DMUSASHI_CNF='"bmlisa_m68kconf.h"' -include $(ROOT)/src/port/idle_port.h

OBJS := $(addprefix $(OUT)/idle/,$(IDLE_SRCS:.c=.o)) \
        $(addprefix $(OUT)/musashi/,$(MUSASHI_SRCS:.c=.o) $(MUSASHI_GEN_SRCS:.c=.o)) \
        $(OUT)/port/lisa_cpu.o $(OUT)/port/lisa_nofpu.o $(OUT)/port/lisa.o $(OUT)/port/scc.o \
        $(OUT)/port/platform.o $(OUT)/port/keymap.o \
        $(OUT)/port/menu.o $(OUT)/port/osd.o \
        $(OUT)/host_sim.o $(OUT)/live.o

all: $(OUT)/bmlisa_sim

$(OUT)/bmlisa_sim: $(OBJS)
	$(CXX) -o $@ $(OBJS) -lm

# Musashi's opcode tables.
$(OUT)/musashi/m68kmake: $(MUSASHI)/m68kmake.c
	@mkdir -p $(dir $@)
	@$(CC) -O1 -w -o $@ $<
$(OUT)/musashi/m68kops.c $(OUT)/musashi/m68kops.h: $(OUT)/musashi/m68kmake $(MUSASHI)/m68k_in.c
	@$< $(OUT)/musashi $(MUSASHI)/m68k_in.c > /dev/null

$(OUT)/musashi/m68kops.o: $(OUT)/musashi/m68kops.c $(ROOT)/src/port/bmlisa_m68kconf.h
	@$(CC) $(MUSASHI_FLAGS) -c -o $@ $<
$(OUT)/musashi/%.o: $(MUSASHI)/%.c $(OUT)/musashi/m68kops.h $(ROOT)/src/port/bmlisa_m68kconf.h
	@mkdir -p $(dir $@)
	@$(CC) $(MUSASHI_FLAGS) -c -o $@ $<

# BMLisa's files that use Musashi's internals.
$(OUT)/port/lisa_cpu.o $(OUT)/port/lisa_nofpu.o: $(OUT)/port/%.o: $(ROOT)/src/port/%.c \
                        $(ROOT)/src/port/idle/cpu/lisa_cpu.h $(ROOT)/src/port/bmlisa_m68kconf.h \
                        $(OUT)/musashi/m68kops.h
	@mkdir -p $(dir $@)
	$(CC) $(MUSASHI_FLAGS:-w=) -I $(ROOT)/third_party -Wall -c -o $@ $<

$(OUT)/idle/%.o: $(IDLE)/%.c $(ROOT)/src/port/idle_port.h $(wildcard $(ROOT)/src/port/idle/cpu/*.h)
	@mkdir -p $(dir $@)
	@$(CXX) $(IDLE_FLAGS) $(IDLE_CFLAGS) -c -o $@ $<

$(OUT)/port/%.o: $(ROOT)/src/port/%.cpp $(ROOT)/src/port/lisa.h $(ROOT)/src/port/idle_port.h \
                 $(wildcard $(ROOT)/src/port/idle/cpu/*.h)
	@mkdir -p $(dir $@)
	$(CXX) $(IDLE_FLAGS) -Wall -c -o $@ $<

$(OUT)/port/%.o: $(ROOT)/src/port/%.c $(ROOT)/src/port/bmlisa.h $(ROOT)/src/port/lisa.h \
                 $(ROOT)/src/port/osd.h $(ROOT)/src/host.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Wall -Wextra -Wno-unused-parameter -c -o $@ $<

$(OUT)/host_sim.o: host_sim.c live.h $(ROOT)/src/host.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Wall -c -o $@ $<

$(OUT)/live.o: live.c live.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Wall -Wextra -DLIVE_HTML=\"$(abspath live.html)\" -c -o $@ $<

clean:
	rm -rf $(OUT)

.PHONY: all clean
