#!/usr/bin/env python3
"""Derive VUC's real iostream startup cleanup entries from its original ELF.

No candidate bytes or addresses are invented: each 16-byte __tcf_0 symbol
must have the expected compiler-generated ABI and a named _ZSt8__ioinit
object in the authentic ELF symbol table.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("vuc_match_probe",
                                               ROOT / "vuc/scripts/match_probe.py")
match = importlib.util.module_from_spec(spec)
spec.loader.exec_module(match)
SOURCE = "vuc/src/IostreamInit.cpp"
DEST = ROOT / "vuc/config"
D1_PLT = 0x080499dc
INIT_PLT = 0x0804974c
ATEXIT_PLT = 0x0804972c
CONSTANTS_HELPER = 0x08049af0
PREFIX = bytes.fromhex("5589e5c74508")
SUFFIX = bytes.fromhex("5de9")


def derive(raw, target):
    code = next(sec for sec in target["sections"] if sec["name"] == ".text")
    valid = []
    for symbol in target["functions"]:
        if symbol["name"] != "__tcf_0" or symbol["size"] != 16:
            continue
        addr = symbol["address"]
        start = code["offset"] + addr - code["address"]
        body = raw[start:start + 16]
        if len(body) != 16 or body[:6] != PREFIX or body[10:12] != SUFFIX:
            raise ValueError("unrecognized original 16-byte __tcf_0 at " + hex(addr))
        obj = struct.unpack_from("<I", body, 6)[0]
        displacement = struct.unpack_from("<i", body, 12)[0]
        destination = addr + 16 + displacement
        if destination != D1_PLT:
            raise ValueError("unexpected original cleanup target at " + hex(addr))
        objects = {".bss": {"_ZSt8__ioinit": "0x%08x" % obj}}
        match.verify_original_objects(raw, target, objects)
        valid.append({
            "function": "__tcf_0",
            "address": "0x%08x" % addr,
            "size": 16,
            "source": SOURCE,
            "flags": ["-O2", "-frename-registers", "-fuse-cxa-atexit"],
            "relocations": {"_ZNSt8ios_base4InitD1Ev": "0x%08x" % D1_PLT},
            "relocation_objects": objects,
        })
    return valid


def path_for(record):
    return DEST / ("iostream_cleanup_%s_probe.json" %
                   record["address"].removeprefix("0x"))


def derive_initializers(raw, target):
    """Recover independent 66-byte ios_base::Init constructor helpers."""
    code = next(sec for sec in target["sections"] if sec["name"] == ".text")
    cleanup_addresses = {x["address"] for x in target["functions"]
                         if x["name"] == "__tcf_0" and x["size"] == 16}
    candidates = []
    for symbol in target["functions"]:
        if (symbol["name"] != "_Z41__static_initialization_and_destruction_0ii"
                or symbol["size"] != 66):
            continue
        addr = symbol["address"]
        loc = code["offset"] + addr - code["address"]
        body = raw[loc:loc + 66]
        if (len(body) != 66 or body[0:6] != bytes.fromhex("5589e583ec18")
                or body[25:28] != bytes.fromhex("c70424")
                or body[32] != 0xe8 or body[37:40] != bytes.fromhex("c70424")
                or body[59] != 0xe8):
            raise ValueError("unrecognized original static initializer at " + hex(addr))
        object_address = struct.unpack_from("<I", body, 28)[0]
        callback_address = struct.unpack_from("<I", body, 40)[0]
        init_address = addr + 37 + struct.unpack_from("<i", body, 33)[0]
        atexit_address = addr + 64 + struct.unpack_from("<i", body, 60)[0]
        if (init_address != INIT_PLT or atexit_address != ATEXIT_PLT
                or callback_address not in cleanup_addresses):
            raise ValueError("unverified original constructor/callback at " + hex(addr))
        objects = {".bss": {"_ZSt8__ioinit": "0x%08x" % object_address}}
        match.verify_original_objects(raw, target, objects)
        if addr == CONSTANTS_HELPER:
            continue  # already accepted from its own named-global source
        candidates.append({
            "function": "_Z41__static_initialization_and_destruction_0ii",
            "address": "0x%08x" % addr,
            "size": 66,
            "source": SOURCE,
            "flags": ["-O2", "-frename-registers", "-fuse-cxa-atexit",
                      "-ffunction-sections", "-falign-jumps=1"],
            "relocations": {
                "_ZNSt8ios_base4InitC1Ev": "0x%08x" % INIT_PLT,
                ".text.__tcf_0": "0x%08x" % callback_address,
                "__dso_handle": "0x08077004",
                "__cxa_atexit": "0x%08x" % ATEXIT_PLT,
            },
            "relocation_objects": objects,
        })
    return candidates


def initializer_path_for(record):
    return DEST / ("iostream_static_%s_probe.json" %
                   record["address"].removeprefix("0x"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true")
    args = parser.parse_args()
    raw, target = match.decomp.load()
    records = derive(raw, target)
    initializers = derive_initializers(raw, target)
    if args.write:
        for record in records:
            output = path_for(record)
            output.write_text(json.dumps(record, indent=2) + "\n")
        print("wrote %d original-address-specific callback configs" % len(records))
        for record in initializers:
            initializer_path_for(record).write_text(json.dumps(record, indent=2) + "\n")
        print("wrote %d additional 66-byte static initializer configs" % len(initializers))
    else:
        print(json.dumps({"callbacks": records, "initializers": initializers}, indent=2))
    if len(records) != 20:
        raise ValueError("expected 20 original 16-byte iostream callbacks")
    if len(initializers) != 13:
        raise ValueError("expected 13 additional original 66-byte iostream helpers")


if __name__ == "__main__":
    main()
