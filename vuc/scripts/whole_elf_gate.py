#!/usr/bin/env python3
"""Require byte-for-byte equality with the authenticated original VUC ELF32.

This is the *final* executable-level gate, separate from the existing strict
function matcher. A PASS only establishes byte equality for the supplied
candidate file, not that its build provenance is independently authentic.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import decomp


def compare(original: bytes, candidate: bytes) -> dict:
    """Compare all bytes, including ELF headers, sections, padding and data."""
    minimum = min(len(original), len(candidate))
    mismatch = next((i for i in range(minimum) if original[i] != candidate[i]), None)
    if mismatch is None and len(original) != len(candidate):
        mismatch = minimum

    original_hash = hashlib.sha256(original).hexdigest()
    candidate_hash = hashlib.sha256(candidate).hexdigest()
    accepted = mismatch is None and original == candidate
    return {
        "acceptance": "PASS" if accepted else "FAIL",
        "whole_elf_byte_exact": accepted,
        "original_sha256": original_hash,
        "candidate_sha256": candidate_hash,
        "original_size": len(original),
        "candidate_size": len(candidate),
        "first_different_file_offset": mismatch,
    }


def check_candidate(candidate_path: Path) -> dict:
    # decomp.load() checks the canonical ZIP member, original SHA-256 and ELF
    # identity independently before returning the trusted reference bytes.
    original, info = decomp.load()
    result = compare(original, candidate_path.read_bytes())
    result["original"] = info["target"]
    result["candidate"] = str(candidate_path)
    return result


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate", type=Path, required=True,
                        help="path to a locally rebuilt VUC ELF candidate")
    args = parser.parse_args(argv)
    try:
        result = check_candidate(args.candidate)
    except (OSError, ValueError) as exc:
        print("whole ELF comparison could not run: " + str(exc), file=sys.stderr)
        return 2

    print(json.dumps(result, indent=2))
    return 0 if result["whole_elf_byte_exact"] else 1


if __name__ == "__main__":
    sys.exit(main())
