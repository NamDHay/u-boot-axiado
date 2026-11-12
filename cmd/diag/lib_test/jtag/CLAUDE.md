# lib_test/jtag — `ax_jtag` Host JTAG diagnostics

The AX3005 has a **Host JTAG controller** (Arm CSS600 JTAG-AP in the DSUB block). With it, U-Boot on the A53 acts as a JTAG **master**: it drives TCK/TMS/TDI/TRST to an external target's TAP chain. On top of that, this directory implements an ADIv5/ADIv6 DAP client so it can debug the target's cores (M0, R52 core 0, A53 core 0 now; other cores, A35 and R4 later).

Origin: ported from the bare-metal `~/jtag/jtag_cmd.c` + `~/jtag/ax_host_jtag.c` (see `~/jtag/CLAUDE_JTAG.md`), but rebuilt on top of the `ax_jtag_ops` HAL instead of calling the controller directly.

## Layers (bottom → top)

| Layer | File | Notes |
|---|---|---|
| HAL ops table | `hal/jtag/ax_jtag.h` | `struct ax_jtag_ops jtag` + `ax_jtag_*()` inline wrappers (return `-ENOSYS` if the op is NULL). |
| Controller driver | `hal/jtag/axiado_jtag.c` | Fills `jtag`. Always built (`obj-y` in `hal/Makefile`). JTAG window `AX3000_CSR_BASE_ADRS_DSUB + 0x0000`; HOST_JTAG CSR = `+0x1C14` (DSUB_LOCAL + 0x14). |
| TAP + DAP library | `jtag_dap.c/.h` | IR/DR scans via `ax_jtag_xfer()`, IDCODE, DAP chain, DP/AP/MEM-AP access. |
| Target descriptor | `jtag_target.h` | `struct ax_jtag_target`: name, debug `ap`, `mem_ap` (for md/mw/load/verify, 0 = `ap`), `reg64`, ops (probe/halt/run/step/state/regs/reg_read/reg_write/reset/mem_check). Register ops take `u64`. |
| ARMv8 debug core | `armv8_dbg.c/.h` | Shared ARMv8 external debug (EDSCR/EDITR/DTR + CTI), driven by a `struct armv8_core` (name, AP, debug base, CTI base, `aarch64`). |
| Targets | `m0.c`, `r52.c`, `a53.c` | One file per core. `r52.c`/`a53.c` only hold a core-0 descriptor and wrap `armv8_*()`. |
| CLI | `jtag_cli.c` | `ax_jtag` command; target table `ax_jtag_targets[]`. |

Build: `CONFIG_CMD_AX_JTAG` (in `configs/ax3005_scm3005_defconfig`) → `lib_test/Makefile` builds `jtag_cli.o jtag_dap.o m0.o armv8_dbg.o r52.o a53.o`.

## HAL transfer semantics (`ax_jtag_xfer`)

- Every xfer starts **and ends** in Run-Test/Idle: SIR = RTI→Shift-IR→shift (TMS=1 on last bit)→Update-IR→RTI; SDR is the same for DR.
- Data buffer is LSB-first, little-endian bytes. `direction & JTAG_READ_XFER` copies TDO back into the same buffer. SIR never returns TDO.
- `ax_jtag_enable()` = enable HOST_JTAG, PSEL=1, **TRST pulse + TLR → RTI**. Any enable/`status_set(FORCE_RESET)`/TRST **drops the DAP daisy chain**. The CLI calls `dap_invalidate()` for these.
- `status_get` always reports IDLE (no hardware tracking). `bitbang` does not return TDO.
- The driver `printf`s on every `wait_idle`/TMS/shift, so DAP operations are very verbose. Remove/guard those prints when doing bulk memory transfers.
- Unverified: the shift header writes `length = bits-1` into a single byte with bit 7 free. It is unconfirmed that CSS600 accepts more than 128 bits per command, even though the driver allows 256. Keep scans ≤ 64 bits (the library already does).

## TAP chain / IDCODE flow

1. After TLR the master TAP's DR = IDCODE → 32-bit DR scan = **master IDCODE**. `0xFFFFFFFF` = TDO floating; LSB=0 = BYPASS.
2. Enable the DAP TAP: master IR (5 bits) = `T_USER0 0x10`, DR (32 bits) = `0x800` (`tdr_secondary_tap_sel = 1`).
3. Chain IR is now **9 bits** = DAP IR (4 bits, TDO side) | master IR (5 bits).
   - IDCODE in both = `0x09E` → 64-bit DR: low 32 = **DAP IDCODE**, high 32 = master IDCODE (must match step 1, otherwise the secondary-TAP select didn't take).
   - `DPACC = 0x1FA`, `APACC = 0x1FB` (master in BYPASS → 1 extra DR bit).
4. **No TLR after step 2**, or the chain collapses back to the master TAP only.

## DAP client (jtag_dap.c)

- DPACC/APACC scan = 3 bits (RnW, A[3:2]) + 32 data + 1 master BYPASS bit (= 36 bits) + `s_tdo_lag`. TDO returns the **previous** scan's ACK/data, so `dap_op()` issues the request and then flushes with a `RDBUFF` read.
- ACK: this DAP returns **`0b100` (4) for OK**, not ADIv5 `0b010`. Both are accepted. `1` = WAIT (retried up to 100×).
- `s_tdo_lag` is auto-detected in `dap_open()` by reading DPIDR (bit 0 must be 1), trying lag 1 then lag 0. Expected DPIDR `0x4C013477`.
- Power-up: CTRL/STAT ← `0x54000000`, poll for `0xA0000000` ACK bits; expected readback `0xF4000000`. Sticky clear = write `| 0x32`.
- ADIv6 AP addressing: SELECT ← `(ap + off) & ~0xF`, then APACC with A[3:2] = `(off >> 2) & 3`. MEM-AP: CSW `0xD00`, TAR `0xD04`, DRW `0xD0C`, IDR `0xDFC`.
- `mem_ap_open(ap)` opens the DAP if needed, checks IDR ≠ 0 and sets CSW to 32-bit with no auto-increment. It caches the open DAP and up to 4 opened APs (R52/A53 alternate between their debug APB-AP and the AXI-AP); `dap_invalidate()` resets that cache.
- `mem_write_block()` / `mem_read_block()`: bulk transfers with CSW AddrInc=single. SELECT is written once (CSW/TAR/DRW share bank 0xD00), TAR is reloaded at every 1 KB boundary (auto-inc wrap limit), and CSW is restored to no-increment afterwards. The AP must be open.

## AP map on the target (from the reference code)

From the reference code and the J-Link scripts in `~/jtag/segger` (`CORESIGHT_AddAP` lines):

| AP | Base | Used for |
|---|---|---|
| PERIPH APB-AP | `0x2000` | A53 cluster: core N debug `0x810000 + N*0x100000`, CTI `0x820000 + N*0x100000` (`a53_core_N.jlinkscript`) |
| AXI MEM-AP | `0x4000` | System memory (image loads to SRAM/DDR); `mem_ap` of r52/a53 (`JTAG_AXI_MEM_AP`) |
| M0 AHB-AP | `0x6000` | Cortex-M0 + its SCS debug registers |
| ML APB-AP | `0x8000` | ML A53 cluster, core 0 debug `0x810000`, CTI `0x820000` (`a53ml_core_N.jlinkscript`), not wired up yet |
| RT660 JTAG-AP | `0x1000` | RT660 JTAG, not used |
| SVM APB-AP | `0x1C000` | R52 core0 debug `0x810000`, CTI `0x820000`, ETM `0x840000` (`svm_r52.jlinkscript`) |

Expected IDs from the scripts: master TAP IDCODE `0x10200BC1`, DAP IDCODE `0x4BA06477`, DPIDR `0x4C013477`.

## Cortex-M0 (m0.c), ARMv6-M

- DHCSR `0xE000EDF0` (write key `0xA05F0000`; C_DEBUGEN b0, C_HALT b1, C_STEP b2, C_MASKINTS b3; S_REGRDY b16, S_HALT b17, S_SLEEP b18, S_LOCKUP b19, S_RETIRE_ST b24, S_RESET_ST b25), DCRSR `0xE000EDF4` (REGWnR b16), DCRDR `0xE000EDF8`, DEMCR `0xE000EDFC` (VC_CORERESET b0), DFSR `0xE000ED30`, AIRCR `0xE000ED0C` (`0x05FA0004` = SYSRESETREQ), CPUID `0xE000ED00` (part `0xC20` M0, `0xC60` M0+).
- REGSEL: 0–12 = R0–R12, 13 SP, 14 LR, 15 DebugReturnAddress (PC), 16 xPSR, 17 MSP, 18 PSP, 20 CONTROL/PRIMASK. Register access requires the core to be halted.
- In the reference flow, **memory writes through the M0 AP need the M0 halted**. `mem_check(true)` enforces this for `mw`.

## ARMv8 cores (armv8_dbg.c): R52 core 0, A53 core 0

- External debug registers (from the core debug base): EDECR `0x024` (SS b2), DBGDTRRX `0x080`, EDITR `0x084`, EDSCR `0x088`, DBGDTRTX `0x08C`, EDRCR `0x090` (CSE b2, CSPA b3), OSLAR `0x300`, EDPRSR `0x314` (PU b0, SPD b1, R b2, HALTED b4, OSLK b5, DLK b6, SDR b11), MIDR `0xD00`, LAR `0xFB0`, DEVARCH `0xFBC`.
- EDSCR: STATUS[5:0] (`0x01` restarting, `0x02` running, anything else = halted, e.g. `0x13` external debug request, `0x1B` step), ERR b6, EL[9:8], RW[13:10], HDE b14, SDD b16, INTdis[23:22], ITE b24, TXfull b29, RXfull b30.
- Setup (`core_setup`, run before probe/halt/run/step): unlock debug + CTI (`0xC5ACCE55` → LAR), OSLAR ← 0, read EDPRSR (fails with `-ENODEV` if PU = 0), set EDSCR.HDE, CTICONTROL ← 1, CTIOUTEN0 ← ch0 (halt), CTIOUTEN1 ← ch1 (restart), CTIGATE ← 3 (value from the working reference flow).
- Halt: CTIINTACK ← 1, CTIAPPPULSE ← ch0, wait EDSCR halted, then CTIINTACK ← 1 to ack the halt trigger. Run: EDRCR ← CSE|CSPA, CTIINTACK ← 1, wait CTITRIGOUTSTATUS[0] = 0, CTIAPPPULSE ← ch1, wait STATUS = running. Step: EDECR.SS = 1 and EDSCR.INTdis = 3, restart, wait halted, then restore both.
- Instruction execution: wait ITE, write EDITR, wait ITE, check ERR (clear with EDRCR.CSE). The execution state is chosen per halt: R52 is always AArch32; A53 uses `EDSCR.RW >> EDSCR.EL` & 1 (1 = AArch64).
  - AArch32 executes **T32** in Debug state. EDITR takes the first halfword in [15:0], so opcodes are halfword-swapped (`T32_ITR`). The reference code had an A32/T32 fallback (`s_itr_swap`); here it is always T32. `MOV PC, R0` (A32) from the reference is not used; PC is set through DLR.
  - AArch32 registers: R0–R14 via `MCR/MRC p14,0,Rt,c0,c5,0` (DTRTX/DTRRX); 15 = PC via DLR (`p15,3,c4,c5,1`), 16 = CPSR via DSPSR (`p15,3,c4,c5,0`). R0 is the scratch register and is saved and restored.
  - AArch64 registers: X0–X30 via `MSR/MRS DBGDTR_EL0` (read: DTRRX = [63:32], DTRTX = [31:0]; write: DTRTX ← hi, DTRRX ← lo); 31 = SP (`ADD X0, SP, #0` / `ADD SP, X0, #0`), 32 = PC (DLR_EL0), 33 = PSTATE (DSPSR_EL0). X0 is the scratch register.
- Memory (`md`/`mw`/`load`/`verify`) goes through the AXI MEM-AP `0x4000`, not the core. `mem_check(true)` requires the core to be halted before writes (the reference flow halts the R52 before loading). Caches of the target core are not maintained, so invalidate them in the loaded image or reset the core before running code loaded over the AXI-AP.
- `reset` is not implemented for R52/A53 (EDPRCR.CWRR / EDECR.RCE wiring on this SoC is unconfirmed).
- **More cores later**: add another `struct armv8_core` (A53 core N: debug `0x810000 + N*0x100000`, CTI `0x820000 + N*0x100000`; ML A53: AP `0x8000`) and a target that wraps it. For SMP halt/restart across cores, route the same CTI channels on every core's CTI and keep CTIGATE open; with only core 0 configured, the other cores' CTIOUTEN stay 0 and are not affected.

## Commands

```
ax_jtag enable [tck_hz] | disable | freq [tck_hz] | trst <0|1>
ax_jtag idcode [tck_hz]          # master + DAP IDCODE
ax_jtag dap                      # DPIDR, power-up, CTRL/STAT
ax_jtag apidr <ap_hex>           # e.g. 6000
ax_jtag targets
ax_jtag m0 probe|halt|run|step|state|regs
ax_jtag m0 reg <n> [val_hex]
ax_jtag m0 reset [halt]
ax_jtag m0 md <addr_hex> [count]
ax_jtag m0 mw <addr_hex> <val_hex> [count]
ax_jtag m0 load <dst_hex> [size_hex] [src_hex]    # DDR -> target
ax_jtag m0 verify <dst_hex> [size_hex] [src_hex]  # compare target with DDR
ax_jtag r52|a53 probe|halt|run|step|state|regs    # same set for the ARMv8 cores
ax_jtag r52|a53 reg <n> [val_hex]                 # see register numbers above
ax_jtag r52|a53 md|mw|load|verify ...             # through the AXI-AP 0x4000
```

`load`/`verify`: `src` defaults to `JTAG_LOAD_SRC_ADDR` = `AX3000_MEM_BASE_ADRS_DDR_LO + 0x10000000` and `size` to `$filesize`, so the usual flow is: stage the image in DDR (`tftp`/`fatload`/`loady` to that address), `ax_jtag m0 halt`, `ax_jtag m0 load <dst>`, `ax_jtag m0 verify <dst>`, then set SP/PC (`reg 13`, `reg 15`) and `run`. The load goes through `mem_check(true)`, so the M0 must be halted. `dst` must be 4-byte aligned; a trailing partial word is merged with the target's existing bytes. Transfers run in 1 KB chunks with progress output and Ctrl-C.
After a successful `load`, the source DDR range is flushed from the A53 D-cache (`flush_cache`, widened to `ARCH_DMA_MINALIGN` lines) so DDR holds the staged image. The JTAG writes go straight to the bus and the M0 has no cache, so the destination is not touched; a destination in A53-visible DDR would need an invalidate instead.

Bring-up order: `idcode` → `dap` → `apidr 6000` → `m0 probe` → `m0 state` → `m0 halt` → `m0 regs`.
R52: `apidr 1C000` → `apidr 4000` → `r52 probe` (MIDR part `0xD13`) → `r52 halt` → `r52 regs`. A53: the same with `apidr 2000` and `a53` (MIDR part `0xD03`).
Loading an R52/A53 image: `<core> halt` → `<core> load <dst>` → `<core> verify <dst>` → `<core> reg 15 <entry>` (A53 in AArch64: `reg 32`) → `<core> run`.

## Debug checklist (lowest layer first)

1. JTAG-AP IDR = `0x24760020` (printed by enable) and CSW.PORTCONNECTED = 1 (bit 3 at JTAG `+0xD00`; the HAL does not expose this yet).
2. Master IDCODE valid → wiring/TCK are OK. If not, lower TCK (`freq`) and check TRST, target power and reset.
3. DAP IDCODE valid and the master IDCODE unchanged → the secondary-TAP select worked.
4. DPIDR valid → TDO lag is right. All-ones/zeros means the DAP is not in the chain or a TLR happened.
5. CTRL/STAT power-up ACK. If an ACK other than 4/1 comes back, clear sticky errors (`ax_jtag dap`) and retry.
6. AP IDR ≠ 0, then the target's debug registers.

## Adding a new target (r52, a53, a35, r4 ...)

1. Create `<core>.c` here. Define its AP base and debug register map, then implement the ops using only `jtag_dap.h` (`mem_ap_open`, `mem_read32`, `mem_write32`, `ap_read`, ...). Leave unsupported ops NULL.
2. Export `const struct ax_jtag_target ax_jtag_target_<core>` and declare it in `jtag_target.h`.
3. Add `&ax_jtag_target_<core>` to `ax_jtag_targets[]` in `jtag_cli.c`.
4. Add `jtag/<core>.o` to the `CONFIG_CMD_AX_JTAG` line in `lib_test/Makefile`.
5. ARMv8 cores (A35, other A53/R52 cores): no new debug code is needed. Add a `struct armv8_core` descriptor and wrap the `armv8_*()` helpers as in `r52.c`/`a53.c`, setting `mem_ap = JTAG_AXI_MEM_AP` and `reg64` for AArch64-capable cores. ARMv7-R (R4) uses the older DBGDSCR/DBGITR register map and needs its own file.
