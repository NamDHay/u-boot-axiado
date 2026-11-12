# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Upstream U-Boot plus an Axiado port. Axiado-specific work lives in:

- `arch/arm/mach-axiado/` (SoC Kconfig: `AXIADO_AX3005`, `AXIADO_AX3000`, `AXIADO_MK`) and `board/axiado/{scm3005,scm3000,mk}/`. Each board has its own `u-boot.lds` (`CONFIG_SYS_CUSTOM_LDSCRIPT`).
- `configs/ax3005_scm3005_defconfig` (primary target), `ax3000_scm3000_defconfig`, `mk_defconfig`, plus `include/configs/<board>.h`.
- `mk` is **not** the mK SoC any more. It is an SCM3000 (AX3000) clone that runs from the 1 MiB A53 OCM without relocation, and `CONFIG_AXIADO_MK` takes the AX3000 code paths in `cmd/diag`. See `board/axiado/mk/CLAUDE.md`.
- `cmd/diag/`: the bare-metal **diagnostics framework**. Most active development happens here.

The rest of the tree is upstream U-Boot. Upstream does not accept GitHub PRs (`.github/pull_request_template.md` points to the mailing-list patch flow), but this fork's own work goes through GitHub PRs on `master`.

## Build

The toolchain is Debian `aarch64-linux-gnu-` (GCC 14). The build is in-tree, and the build products (`u-boot.bin`, `u-boot-fit-dtb.bin`, `*.o`, `*.su`) sit in the source tree.

```sh
make ax3005_scm3005_defconfig          # or ax3000_scm3000_defconfig / mk_defconfig
make CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc)
make CROSS_COMPILE=aarch64-linux-gnu- cmd/diag/   # rebuild just the diag objects
make savedefconfig && cp defconfig configs/ax3005_scm3005_defconfig  # persist Kconfig changes
```

There are no host-side unit tests for the Axiado code. You verify changes by running the commands on the board at the `Axiado> ` prompt. Upstream checks (`scripts/checkpatch.pl`, `test/py`) exist, but the diag code doesn't follow kernel style strictly: it mixes 4-space and tab indentation.

## `cmd/diag` architecture

The code has three layers. Every directory under `cmd/diag` is on the include path (the Makefiles use `find ... -type d` → `-I`), so headers are included by bare name (`#include "ax_gpio.h"`).

1. **HAL** (`cmd/diag/hal/<ip>/`): each IP has
   - `ax_<ip>.h`: `struct ax_<ip>_ops` plus `extern struct ax_<ip>_ops <ip>;` and `static inline ax_<ip>_*()` wrappers that return `-ENOSYS` when an op is NULL.
   - `ax_<ip>.c`: a `__weak struct ax_<ip>_ops <ip>;` definition, so the build links even when no driver is selected.
   - a vendor driver (`cdns_*.c`, `inno_*.c`, `dwc_*.c`, `arasan_*.c`, `axiado_*.c`) that defines the strong `struct ax_<ip>_ops <ip> = {...}`. It is gated by a driver Kconfig (e.g. `CDNS_GPIO`, `INNO_PCIE`) in `hal/Makefile`.
   - Drivers poke registers directly. Base addresses come from `hal/base/{Ultra,A0,mK}/ax3000_*base_adrs.h`, which `ax_diag.h` selects by SoC config (Ultra = AX3005).
2. **CLI commands** (`cmd/diag/lib_test/<ip>/<ip>_cli.c`): a `U_BOOT_CMD(ax_<ip>, ...)` built on the HAL wrappers, gated by `CONFIG_CMD_AX_<IP>` (all depend on `CMD_AX_DIAG`; see `cmd/diag/Kconfig`).
3. **Test runner** (`ax_diag.c`, `diag_common.c`): the `ax_diag [run|status|clear] [test...]` command. Tests are the static table `diag_list[]` in `diag_common.c`. Each group (wrapped in `#ifdef CONFIG_CMD_AX_<IP>`) lists testcases as `{ "cmd1; cmd2", timeout_ms, 0 }`. The commands are ordinary U-Boot command strings (usually the `ax_<ip>` CLI). A failing `;` sub-command skips the rest of its testcase.
   - **Persistent result record** at `CONFIG_CMD_AX_DIAG_RECORD_ADDR` (default `0xF0000000`). The record is flushed from D-cache to DRAM before each command runs, so after a reset a still-`RUNNING` entry is reported as `CRASH`. The record holds a CRC of `diag_list[]` (the "layout"), so any edit to the test table invalidates old records. Bump `DIAG_REC_VERSION` if `struct diag_record` changes.
   - **Abort/timeout** (`CONFIG_CMD_AX_DIAG_ABORT`): a cyclic hook checks Ctrl+C and the testcase timeout, then `longjmp`s back to the runner. It only fires from code that calls `udelay`/`mdelay`/`schedule()`, so a tight register-poll loop without a delay can't be aborted.
   - Limits: `DIAG_MAX_GROUP` = 32 groups, `DIAG_MAX_TEST` = 32 testcases per group.

### Adding a new IP test

To add an IP test:

1. Add the HAL header, the weak `.c`, and the driver.
2. Add the `hal/Makefile` entries.
3. Add the `CMD_AX_<IP>` and driver configs to `cmd/diag/Kconfig`.
4. Add `lib_test/<ip>/<ip>_cli.c` and its `lib_test/Makefile` line.
5. Optionally add a `diag_list[]` group.
6. Enable the new configs in the defconfig.

## Subdirectory docs

Some subdirectories have their own `CLAUDE.md` with hardware details (for example `board/axiado/mk/CLAUDE.md` for the OCM memory layout and size budget, and `cmd/diag/lib_test/jtag/CLAUDE.md`, which covers the host JTAG/DAP client, its TAP-chain quirks, and the AP map). Read the matching one before working in a directory. When you change code, update the `CLAUDE.md` of the directory you changed in the same turn.
