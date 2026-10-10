#!/usr/bin/env python3
"""Compile a VUC-era C++ fingerprint using an explicitly supplied compiler.

This is evidence collection, not byte matching. No compiler is downloaded.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "vuc/probes/compiler_fingerprint.cpp"


def version(command):
    result = subprocess.run([command, "--version"], check=True, capture_output=True, text=True)
    return result.stdout.splitlines()[0] if result.stdout else ""


def run_probe(compiler, output, flags):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    compiler_version = version(compiler)
    is_historical = "3.3.3" in compiler_version and "GCC" in compiler_version.upper()
    candidates = {}
    for name, options in flags.items():
        obj = output / (name + ".o")
        command = [compiler, *options, "-c", str(SOURCE), "-o", str(obj)]
        result = subprocess.run(command, text=True, capture_output=True)
        record = {"command": command, "exit_code": result.returncode,
                  "stderr": result.stderr[-2000:]}
        if result.returncode == 0:
            record["sha256"] = hashlib.sha256(obj.read_bytes()).hexdigest()
            record["size"] = obj.stat().st_size
        candidates[name] = record
    manifest = {"source": str(SOURCE.relative_to(ROOT)),
                "source_sha256": hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
                "compiler": compiler, "compiler_version": compiler_version,
                "gcc_3_3_3_version_candidate": is_historical,
                "historical_toolchain_verified": False, "candidates": candidates,
                "warning": "Object hashes are fingerprints only; no source-to-original byte matching performed."}
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", required=True, help="explicit path to a C++ compiler")
    parser.add_argument("--out", required=True, help="scratch output directory")
    args = parser.parse_args()
    candidates = {"O0": ["-O0"], "O1": ["-O1"], "O2": ["-O2"],
                  "O2-rename": ["-O2", "-frename-registers"],
                  "O2-omit-fp": ["-O2", "-fomit-frame-pointer"],
                  "Os": ["-Os"]}
    report = run_probe(args.compiler, args.out, candidates)
    print("Compiler:", report["compiler_version"])
    print("Candidates:", sum(bool(v.get("sha256")) for v in report["candidates"].values()),
          "/", len(report["candidates"]))
    print("Historical matching: NOT VERIFIED")
    return 0 if all(v["exit_code"] == 0 for v in report["candidates"].values()) else 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, subprocess.CalledProcessError) as exc:
        print("compiler-probe:", exc, file=sys.stderr)
        sys.exit(1)
