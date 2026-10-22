# Updating IDLE, Musashi and Circle

## IDLE

IDLE is in `third_party/idle`, from its Subversion repository
(`svn.code.sf.net/p/idle-lisa-emu/code`, revision 26, its last), without
the files `src/port/idle.mk` lists as left out: its 68000 (Musashi 3.3,
`cpu/m68k*`) and serial chip (`io/8530scc.c`), whose licences don't allow
BMLisa, and Apple's video PROM. Nothing else in it is changed. If IDLE
changes again:

```
svn export -r <revision> https://svn.code.sf.net/p/idle-lisa-emu/code new-idle
```

copy its files over `third_party/idle` but those, and compare:

| File | What it has to do with IDLE |
| --- | --- |
| `src/port/idle.mk` | The IDLE files built (from its `Makefile.unixsdl`) |
| `src/port/lisa.cpp` | What IDLE's `main_sdl.cpp` did: `MemInit()`, the `init_*` calls, the frame loop (`lisa_loop()`), keys and the mouse |
| `src/port/idle_port.h` | The files IDLE opens by name |
| `src/port/scc.cpp` | `scc_init()`, in place of `io/8530scc.c` |
| `src/port/idle/cpu/`, `lisa_cpu.c` | What IDLE's files use of its 68000 |

## Musashi

Musashi is a git submodule in `third_party/musashi`, from
`https://github.com/kstenerud/Musashi`. To move to a newer one:

```
cd third_party/musashi
git fetch
git checkout <commit>
cd ../..
```

Then compare its `m68kconf.h` with `src/port/bmlisa_m68kconf.h` (options
added), and `src/port/lisa_cpu.c` with its bus error
(`m68ki_exception_bus_error()` in `m68kcpu.h`) and the internals it uses.
If its FPU code calls more SoftFloat functions, the link fails: add them
to `src/port/lisa_nofpu.c`. Run `tools/hostsim/run_tests.sh` with your
ROMs and disks, then commit:

```
git add third_party/musashi src/port docs/THIRD_PARTY.md
git commit -m "Update Musashi to 4.x"
```

## circle-stdlib (Circle and newlib)

circle-stdlib is a submodule in `third_party/circle-stdlib`, pinned to the
same revision BMC64 uses. `make_all.sh` applies the patches in `src/patches/`
to Circle before building:

- `circle_sysconfig_patch.diff` enables multi-core and the USB FIQ
- `circle_linux_bug_patch.diff` turns off `BUG_ON`, whose checks in Circle's
  Linux emulation (used by VCHIQ for display and sound) only allow core 0;
  the emulator calls them from core 1, as BMC64 does
- `circle_bolt_keyboard_patch.diff` adds Logitech Bolt keyboard support
  (from BMC64)

To update, move the submodule to a new circle-stdlib release, run
`git submodule update --init --recursive`, then `./build_sdcard.sh`. If a
patch no longer applies, update it from the matching BMC64 patch or by
hand. `src/fbl.cpp` and `src/vchiqsoundbasedevice.cpp` use Circle's VC4
addon and may need the same changes BMC64 makes when it updates Circle.
