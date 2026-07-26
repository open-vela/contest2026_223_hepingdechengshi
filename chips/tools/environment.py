import os
import json
import argparse


def main():
    parser = argparse.ArgumentParser(
        description=(
            "Initialise build environment variables required by bk_sdk tools "
            "(bk_curr_project.py and friends) when building outside the native "
            "Armino environment.  The four bk_sdk variables are accepted as "
            "explicit CLI arguments, written into os.environ (so any child "
            "processes spawned from this script inherit them), and persisted to "
            "a JSON file and/or a sourceable shell script for the rest of the "
            "build pipeline."
        ),
        epilog="""
examples:
  # Supply the two mandatory variables (PROJECT_DIR and PROJECT_BUILD_DIR):
  %(prog)s \\
      --project-dir   /path/to/boards/bk7258_ap/configs/nsh \\
      --build-dir     /path/to/cmake_out/bk7258_ap_nsh

  # Full set, write JSON and a sourceable shell script:
  %(prog)s \\
      --project-dir   /path/to/boards/bk7258_ap/configs/nsh \\
      --build-dir     /path/to/cmake_out/bk7258_ap_nsh \\
      --soc-name      bk7258_ap \\
      --project-name  nsh \\
      --output        /path/to/cmake_out/env/env.json \\
      --export-sh     /path/to/cmake_out/env/env_setup.sh

  # Diagnostic – print all current environment variables:
  %(prog)s --all

  # Diagnostic – print specific variables:
  %(prog)s --var PYTHONPATH PROJECT_DIR
""",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )

    # ------------------------------------------------------------------ #
    # bk_sdk environment variables (consumed by bk_curr_project.py)       #
    # ------------------------------------------------------------------ #
    sdk_group = parser.add_argument_group(
        "bk_sdk variables",
        "These map 1-to-1 to the os.getenv() calls in bk_curr_project.py.",
    )
    sdk_group.add_argument(
        "--project-dir",
        metavar="PATH",
        help=(
            "Sets PROJECT_DIR.  Root directory of the current project config. "
            "(example: --project-dir $(pwd)/vendor/beken/boards/bk7258_ap/configs/nsh)"
        ),
    )
    sdk_group.add_argument(
        "--custom-project-dir",
        metavar="PATH",
        help=(
            "Sets PROJECT_DIR.  Root directory of the current project config. "
            "(example: --project-dir $(pwd)/vendor/beken/boards/bk7258_ap/configs/nsh)"
        ),
    )
    sdk_group.add_argument(
        "--build-dir",
        metavar="PATH",
        help=(
            "Sets PROJECT_BUILD_DIR.  CMake binary / output directory. "
            "(example: --build-dir $(pwd)/cmake_out/bk7258_ap_nsh)"
        ),
    )
    sdk_group.add_argument(
        "--soc-name",
        metavar="NAME",
        default="",
        help=(
            "Sets ARMINO_SOC_NAME.  SoC identifier string. "
            "(example: --soc-name bk7258_ap)"
        ),
    )
    sdk_group.add_argument(
        "--project-name",
        metavar="NAME",
        default="",
        help=(
            "Sets PROJECT_NAME.  Project / config name. "
            "(example: --project-name nsh)"
        ),
    )
    sdk_group.add_argument(
        "--pythonpath",
        metavar="DIR",
        nargs="+",
        default=[],
        help=(
            "Director(y|ies) to add to PYTHONPATH so the bk_sdk python "
            "modules can be imported.  In the generated shell script these "
            "are prepended to any existing PYTHONPATH. "
            "(example: --pythonpath tools/env_tools/bk_py_libs)"
        ),
    )

    # ------------------------------------------------------------------ #
    # Output options                                                       #
    # ------------------------------------------------------------------ #
    out_group = parser.add_argument_group("output options")
    out_group.add_argument(
        "--output",
        metavar="FILE",
        help=(
            "Write all resolved variables as JSON to FILE. "
            "(example: --output cmake_out/env/env.json)"
        ),
    )
    out_group.add_argument(
        "--export-sh",
        metavar="FILE",
        help=(
            "Write a sourceable shell script to FILE so that shell-level "
            "callers can propagate the variables to their own environment. "
            "(example: --export-sh cmake_out/env/env_setup.sh  "
            "then:  source cmake_out/env/env_setup.sh)"
        ),
    )

    # ------------------------------------------------------------------ #
    # Diagnostic options                                                   #
    # ------------------------------------------------------------------ #
    diag_group = parser.add_argument_group("diagnostic options")
    diag_group.add_argument(
        "--all",
        action="store_true",
        help="Dump all current environment variables (for debugging).",
    )
    diag_group.add_argument(
        "--var",
        metavar="NAME",
        nargs="+",
        help=(
            "Print specific environment variable(s) by name. "
            "(example: --var PYTHONPATH PROJECT_DIR)"
        ),
    )

    args = parser.parse_args()

    # ------------------------------------------------------------------ #
    # Build the mapping of bk_sdk variables and inject into os.environ.   #
    # Child processes spawned from this script will inherit these values. #
    # ------------------------------------------------------------------ #
    sdk_vars: dict = {}
    if args.project_dir is not None:
        sdk_vars["PROJECT_DIR"] = args.project_dir
    if args.build_dir is not None:
        sdk_vars["PROJECT_BUILD_DIR"] = args.build_dir
    if args.soc_name:
        sdk_vars["ARMINO_SOC_NAME"] = args.soc_name
    if args.project_name:
        sdk_vars["PROJECT_NAME"] = args.project_name
    if args.custom_project_dir is not None:
        sdk_vars["CUSTOM_PROJECT_DIR"] = args.custom_project_dir
    # PYTHONPATH is handled with prepend semantics: the new dirs go in front
    # of whatever is already in the environment so we never clobber existing
    # entries.  Tracked separately from sdk_vars because the shell script must
    # reference $PYTHONPATH rather than emit a literal value.
    pythonpath_dirs = list(args.pythonpath)

    for key, value in sdk_vars.items():
        os.environ[key] = value

    if pythonpath_dirs:
        existing = os.environ.get("PYTHONPATH", "")
        combined = os.pathsep.join(pythonpath_dirs + ([existing] if existing else []))
        os.environ["PYTHONPATH"] = combined

    # ------------------------------------------------------------------ #
    # Decide what to serialise / print                                    #
    # ------------------------------------------------------------------ #
    if args.all:
        result = dict(os.environ)
    elif args.var:
        result = {name: os.getenv(name) for name in args.var}
    elif sdk_vars or pythonpath_dirs:
        result = dict(sdk_vars)
        if pythonpath_dirs:
            result["PYTHONPATH"] = os.environ["PYTHONPATH"]
    else:
        # Fallback: legacy behaviour – report PYTHONPATH.
        result = {"PYTHONPATH": os.getenv("PYTHONPATH")}

    # ------------------------------------------------------------------ #
    # Write JSON output                                                    #
    # ------------------------------------------------------------------ #
    if args.output:
        out_dir = os.path.dirname(args.output)
        if out_dir:
            os.makedirs(out_dir, exist_ok=True)
        with open(args.output, "w") as f:
            json.dump(result, f, indent=2)
        print(f"[environment.py] Written to {args.output}")

    # ------------------------------------------------------------------ #
    # Write sourceable shell script                                        #
    # ------------------------------------------------------------------ #
    # Useful for CI pipelines or manual developer setups that need to
    # propagate these variables into the calling shell:
    #   source cmake_out/env/env_setup.sh
    if args.export_sh:
        sh_dir = os.path.dirname(args.export_sh)
        if sh_dir:
            os.makedirs(sh_dir, exist_ok=True)
        with open(args.export_sh, "w") as f:
            f.write("# Auto-generated by environment.py – do not edit manually.\n")
            for key, value in sdk_vars.items():
                # Single-quote the value; escape any embedded single quotes.
                safe_value = value.replace("'", "'\\''")
                f.write(f"export {key}='{safe_value}'\n")
        print(f"[environment.py] Shell script written to {args.export_sh}")

    # ------------------------------------------------------------------ #
    # Print to stdout when no file output was requested                   #
    # ------------------------------------------------------------------ #
    if not args.output and not args.export_sh:
        for key, value in result.items():
            print(f"{key}={value}")


if __name__ == "__main__":
    main()
