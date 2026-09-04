#!/usr/bin/env python3
"""
BK7258 NuttX + CP 固件打包脚本
放置位置: vendor/beken/chips/tools/pack_bk7258.py
输出:     <project_root>/cmake_out/package/all-app.bin  (可直接烧录到 flash 0x0)

用法:
    cd <project_root>
    python3 vendor/beken/chips/tools/pack_bk7258.py

分区布局 (来自 bk_package.json):
  bootloader  @ 0x00000000   68 K
  app.bin(CP) @ 0x00011000 1360 K
  app1.bin(AP)@ 0x00165000 1156 K
"""

import shutil
import subprocess
import sys
from pathlib import Path

# 脚本位于 vendor/beken/chips/tools/，项目根为上 4 级
ROOT = Path(__file__).resolve().parents[4]

OBJCOPY    = ROOT / "prebuilts/gcc/linux-x86_64/arm-none-eabi/bin/arm-none-eabi-objcopy"
AP_ELF     = ROOT / "cmake_out/bk7258_ap_nsh/nuttx"
CP_BIN     = ROOT / "vendor/beken/chips/cp/components/bk_libs/bk7258/app/app.bin"
BOOTLOADER = ROOT / "vendor/beken/chips/cp/components/bk_libs/bk7258/bootloader/normal_bootloader/bootloader.bin"

OUT_DIR = ROOT / "cmake_out/package"
ALL_APP = OUT_DIR / "all-app.bin"

# (文件名, 起始偏移, 分区大小)
PARTITIONS = [
    ("bootloader.bin", 0x00000000,   68 * 1024),
    ("app.bin",        0x00011000, 1360 * 1024),   # CP 核
    ("app1.bin",       0x00165000, 1156 * 1024),   # AP 核 (NuttX)
]

TOTAL_SIZE = PARTITIONS[-1][1] + PARTITIONS[-1][2]   # 0x165000 + 1156K


def objcopy(elf: Path, dst: Path) -> None:
    subprocess.check_call([str(OBJCOPY), "-O", "binary", str(elf), str(dst)])
    print(f"  {elf.name:30s} -> {dst.name}  ({dst.stat().st_size:,} bytes)")


def main() -> None:
    # 前置检查
    bl = BOOTLOADER
    for p, label in [(OBJCOPY, "objcopy"), (AP_ELF, "AP ELF (nuttx)"),
                     (CP_BIN, "CP BIN"), (bl, "bootloader")]:
        if not Path(p).exists():
            sys.exit(f"ERROR: {label} not found:\n  {p}")

    tmp = OUT_DIR / "tmp"
    tmp.mkdir(parents=True, exist_ok=True)

    # 1. 转换 AP ELF 并复制 CP/bootloader 镜像
    print("=== 准备固件镜像 ===")
    objcopy(AP_ELF, tmp / "app1.bin")
    shutil.copy2(CP_BIN, tmp / "app.bin")
    shutil.copy2(bl, tmp / "bootloader.bin")
    print(f"  {'bootloader.bin':30s}    copied  ({(tmp / 'bootloader.bin').stat().st_size:,} bytes)")

    # 2. 拼装 all-app.bin (未覆盖区域填 0xFF)
    print(f"\n=== 拼装 all-app.bin (总大小 {TOTAL_SIZE:,} bytes) ===")
    image = bytearray(b'\xff' * TOTAL_SIZE)

    ok = True
    for fname, offset, part_size in PARTITIONS:
        src = tmp / fname
        data = src.read_bytes()
        if len(data) > part_size:
            print(f"  WARNING: {fname} ({len(data):,} B) 超出分区大小 ({part_size:,} B)!")
            ok = False
        image[offset: offset + len(data)] = data
        print(f"  {fname:20s}  @ 0x{offset:08X}  {len(data):>9,} / {part_size:,} bytes")

    # 32 字节对齐 (与 bk_build_package.py 一致)
    pad = (32 - len(image) % 32) % 32
    image += b'\xff' * pad

    ALL_APP.write_bytes(image)
    print(f"\n{'OK' if ok else 'WARNING'}: {ALL_APP}  ({len(image):,} bytes)")


if __name__ == "__main__":
    main()
