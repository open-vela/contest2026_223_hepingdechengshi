#!/usr/bin/env python3
"""Generate partitions_gen.h from auto_partitions.csv.

Board-level wrapper around the reusable bk_auto_partition / bk_flash_partiton
libraries (plus the bk_flash_denpendecny_generator under tools/build_process).
Unlike the tools/ build_process generator it takes explicit --csv / --out /
--setting arguments, so it can be driven from CMake without the PROJECT_*
environment. Only the header is produced (no OTA / pack side effects).
"""
from __future__ import annotations

import argparse
import json
import logging
import sys
from pathlib import Path

#   parents[0] = scripts/  parents[1] = bk7258_ap/  parents[2] = boards/
CHIPS_DIR = Path(__file__).resolve().parents[3] / "chips"
BK_PY_LIBS = CHIPS_DIR / "tools" / "env_tools" / "bk_py_libs"
BUILD_PROCESS = CHIPS_DIR / "tools" / "build_tools" / "build_process"
DEFAULT_SETTING = BUILD_PROCESS / "bk_sdk" / "smp_flash_partitions_setting.json"

# bk_py_libs provides bk_auto_partition / bk_flash_partiton / bk_misc; the
# build_process dir provides bk_sdk.bk_flash_partitions_generator (which only
# depends on bk_flash_partiton, not on curr_project / PROJECT_* env).
sys.path.insert(0, str(BK_PY_LIBS))
sys.path.insert(0, str(BUILD_PROCESS))

from bk_auto_partition import bk_partitions_table, partition_limit  # noqa: E402
from bk_flash_partiton import bk_flash_partition  # noqa: E402
from bk_sdk.bk_flash_partitions_generator import (  # noqa: E402
    bk_flash_denpendecny_generator,
)

logger = logging.getLogger(Path(__file__).name)


def set_logging() -> None:
    log_format = "[%(name)s|%(levelname)s] %(message)s"
    logging.basicConfig(format=log_format, level=logging.INFO)


def gen_partitions(csv: Path, output: Path, setting_file: Path, crc_enable: bool):
    setting = json.loads(setting_file.read_text()) if setting_file.exists() else {}
    flash_size = setting.get("FLASH_CAPACITY", "8M")

    part_table = bk_partitions_table(csv, flash_size, crc_enable)

    # Apply optional limit / ordering settings (mirrors bk_part._partitions_setting).
    if setting.get("patitions_limit"):
        part_table.set_default_setting(
            [partition_limit(**item) for item in setting["patitions_limit"]]
        )
    if setting.get("internel_partitions"):
        part_table.sort_partitions(setting["internel_partitions"])

    output.parent.mkdir(parents=True, exist_ok=True)

    # gen_partition_json produces the intermediate {"crc_enable", "section"}
    # document consumed by bk_flash_partition. Emit it next to the header in the
    # CMake output dir so the layout is inspectable (matches the original
    # build_process, which writes partitions.json into the partitions dir).
    part_json = output.parent / "partitions.json"
    part_table.gen_partition_json(part_json)
    flash_part = bk_flash_partition(part_json, bk_flash_denpendecny_generator())
    flash_part.gen_partitions_layout_hdr(output)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--csv", type=Path, required=True, help="Path to auto_partitions.csv."
    )
    parser.add_argument(
        "--out", type=Path, required=True, help="Output path for partitions_gen.h."
    )
    parser.add_argument(
        "--setting",
        type=Path,
        default=DEFAULT_SETTING,
        help="Flash partitions setting json (default: bundled "
        "smp_flash_partitions_setting.json).",
    )
    parser.add_argument(
        "--no-crc",
        action="store_true",
        help="Disable flash CRC (default: enabled, matching the SMP project).",
    )
    return parser.parse_args()


def main() -> None:
    set_logging()
    args = parse_args()
    logger.info("Generating %s from %s", args.out, args.csv)
    gen_partitions(args.csv, args.out, args.setting, crc_enable=not args.no_crc)


if __name__ == "__main__":
    main()
