#!/usr/bin/env python3
"""Generate ram_regions.h from a ram_regions.csv table.

Board-level wrapper around the reusable bk_ram_region/bk_misc libraries under
vendor/beken/boards/tools/env_tools/bk_py_libs. Unlike the tools/ build_process
generator, this takes explicit --csv / --out / --setting arguments so it can be
driven directly from CMake without the PROJECT_* environment.
"""
from __future__ import annotations

import argparse
import json
import logging
import sys
from pathlib import Path

# Locate the shared python libraries shipped under the tools/ directory.
#   parents[0] = scripts/  parents[1] = bk7258_ap/  parents[2] = boards/
CHIPS_DIR = Path(__file__).resolve().parents[3]/ "chips"
BK_PY_LIBS = CHIPS_DIR / "tools" / "env_tools" / "bk_py_libs"
DEFAULT_SETTING = (
    CHIPS_DIR
    / "tools"
    / "build_tools"
    / "build_process"
    / "bk_sdk"
    / "smp_ram_setting.json"
)

sys.path.insert(0, str(BK_PY_LIBS))

from bk_misc import parse_format_size  # noqa: E402
from bk_ram_region import bk_ram_region, mem_region  # noqa: E402

logger = logging.getLogger(Path(__file__).name)


def set_logging() -> None:
    log_format = "[%(name)s|%(levelname)s] %(message)s"
    logging.basicConfig(format=log_format, level=logging.INFO)


def gen_ram_regions(ram_regions_table: Path, output: Path, setting_file: Path) -> None:
    ram_regions = bk_ram_region(ram_regions_table)
    with setting_file.open("r") as f:
        def_config = json.load(f)
    sram_addr = int(def_config["SRAM_BASE_ADDR"], 16)
    sram_size = parse_format_size(def_config["SRAM_CAPACITY"])
    psram_addr = int(def_config["PSRAM_BASE_ADDR"], 16)
    psram_size = parse_format_size(def_config["PSRAM_CAPACITY"])
    defconfig: list[mem_region] = []
    for item in def_config["Default_Regions"]:
        defconfig.append(
            mem_region(
                item["name"],
                item["type"],
                int(item["addr"], 16),
                int(item["size"], 16),
            )
        )
    ram_regions.set_sram_setting(sram_addr, sram_size)
    ram_regions.set_psram_setting(psram_addr, psram_size)
    ram_regions.set_default_setting(defconfig)
    output.parent.mkdir(parents=True, exist_ok=True)
    ram_regions.gen_memory_layout_hdr(output)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--csv", type=Path, required=True, help="Path to ram_regions.csv."
    )
    parser.add_argument(
        "--out", type=Path, required=True, help="Output path for ram_regions.h."
    )
    parser.add_argument(
        "--setting",
        type=Path,
        default=DEFAULT_SETTING,
        help="Default ram setting json (default: bundled smp_ram_setting.json).",
    )
    return parser.parse_args()


def main() -> None:
    set_logging()
    args = parse_args()
    logger.info("Generating %s from %s", args.out, args.csv)
    gen_ram_regions(args.csv, args.out, args.setting)


if __name__ == "__main__":
    main()
