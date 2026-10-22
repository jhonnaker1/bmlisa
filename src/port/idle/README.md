# Headers for IDLE

IDLE's files include its 68000's headers as `../cpu/m68k.h` and
`../cpu/m68kcpu.h`. Those were Musashi 3.3, as IDLE changed it, which isn't
in BMLisa (its licence allows non-commercial use only; see
[docs/THIRD_PARTY.md](../../../docs/THIRD_PARTY.md)). With them gone from
`third_party/idle/cpu`, the compiler looks for `../cpu/m68k.h` from each
`-I` folder instead, and BMLisa's build puts `src/port/idle/cpu` first
(see `idle.mk`), so they are found here:

- `cpu/m68k.h`: Musashi 4's `m68k.h` (`third_party/musashi`), with the
  registers IDLE's copy had beyond it.
- `cpu/m68kcpu.h`: the parts of Musashi's internals IDLE's MMU uses (bus
  errors, the supervisor bit).
- `cpu/lisa_cpu.h`: `src/port/lisa_cpu.c`'s functions behind both.

IDLE's files themselves are unchanged.
