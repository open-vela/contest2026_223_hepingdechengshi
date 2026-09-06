# Beken vendor porting notes

This directory is the complete vendor integration for the BK7258 openvela
port. Changes covered by this file must stay under `vendor/beken`; the parent
openvela tree and the other vendor repositories are outside this component's
ownership.

## Layer ownership

- `boards/<board>/` contains board configuration, board startup hooks, pin and
  peripheral wiring, linker scripts, partition/packaging inputs, keys, and
  board-only applications or drivers.
- `chips/bk7258/src/` contains the NuttX-to-Beken entry points and common chip glue
  such as reset/startup, interrupt API, heap setup, UART glue, timers, and
  architecture hooks.
- `chips/bk7258/` is the custom-chip entry selected by
  `CONFIG_ARCH_CHIP_CUSTOM_DIR`; its `include/` and `soc/` directories contain
  BK7258-specific headers and low-level HAL.
- `chips/soc/<soc>/` contains SoC registers, low-level HAL, memory map, and
  SoC linker/toolchain data shared by boards using that SoC.
- `chips/drivers/` contains reusable Beken peripheral drivers. The
  `bk7258_ap` source group is selected by
  `chips/drivers/CMakeLists.txt` through `${target}` and is a valid build
  entry; do not duplicate those sources in the board CMake file.
- `chips/component/` contains SDK components and common initialization stages.
- `chips/common/` contains shared headers, CMSIS, HAL support, and generated
  include wiring. `chips/tools/` contains SDK build and packaging helpers.

The board layer may call chip/component APIs, but chip code must not depend on
one concrete board's startup policy or pin assignment. A function that is a
NuttX board hook, such as `board_late_initialize`, belongs in the selected
board's `src/` file.

## BK7258 build wiring

The normal configuration is
`boards/bk7258_ap/configs/nsh/defconfig`. It selects the custom chip tree at
`chips/` and the board tree at `boards/bk7258_ap/`. NuttX creates the `arch`
target from the chip CMake file and the `board` target from the board CMake
file. Keep these target boundaries explicit:

- Add chip sources and chip include paths to `arch`.
- Add board startup sources and board include paths to `board`.
- If a board source needs a public chip header, include that header directly
  and expose the existing chip include directory through CMake; do not rely on
  an unrelated source file's transitive include.
- Generated partition and RAM headers must be dependencies of the target that
  consumes them, and their input files must live with the board that owns the
  layout.

`board_late_initialize()` performs the late component/module bring-up for the
selected board. Its implementation and all direct declarations belong in
`boards/bk7258_ap/src/`; common reset and NuttX entry flow remains in
`chips/bk7258/src/beken_bringup.c`.

## Change workflow

Run Git commands from `vendor/beken`, inspect `git status` before editing, and
preserve existing user changes. Keep generated output such as `cmake_out/`
out of source changes. Do not build, package, flash, or run hardware tests
unless the request explicitly asks for validation.
