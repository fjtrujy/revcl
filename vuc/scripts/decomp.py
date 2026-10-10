#!/usr/bin/env python3
"""Read-only VUC ELF32 inventory and first-stage decompilation entry point.

Only Python's standard library is required. Matching scores remain unavailable
until the original GCC/binutils toolchain and relocation-aware comparison exist.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
ARCHIVE = ROOT / "archive/upstream/vuc/downloads/X86linux_vuc2004_1.zip"
MEMBER = "X86linux_vuc2004_1/vuc"
CANONICAL_SHA256 = "14c77997057f279ab25a3591c6a379d6e4744f2b1551271d43602948c037d8cf"
CONFIG = ROOT / "vuc/config"
ELF_HEADER = struct.Struct("<16sHHIIIIIHHHHHH")
SHDR = struct.Struct("<IIIIIIIIII")
SYM = struct.Struct("<IIIBBH")


def cstr(data, offset):
    if offset < 0 or offset >= len(data):
        return ""
    end = data.find(b"\0", offset)
    return data[offset:(len(data) if end < 0 else end)].decode("utf-8", "replace")


def load():
    with zipfile.ZipFile(ARCHIVE) as archive:
        data = archive.read(MEMBER)
    digest = hashlib.sha256(data).hexdigest()
    if digest != CANONICAL_SHA256:
        raise ValueError("historical VUC binary SHA-256 mismatch: " + digest)
    header = ELF_HEADER.unpack_from(data)
    ident = header[0]
    if ident[:6] != b"\x7fELF\x01\x01" or header[2] != 3:
        raise ValueError("expected little-endian ELF32 i386")
    shoff, shentsize, shnum, shstrndx = header[6], header[11], header[12], header[13]
    if shentsize != SHDR.size:
        raise ValueError("unexpected section header size")
    shdrs = [SHDR.unpack_from(data, shoff + i * shentsize) for i in range(shnum)]
    strings = data[shdrs[shstrndx][4]:shdrs[shstrndx][4] + shdrs[shstrndx][5]]
    sections = []
    for index, sh in enumerate(shdrs):
        sections.append({"index": index, "name": cstr(strings, sh[0]), "type": sh[1],
                         "flags": sh[2], "address": sh[3], "offset": sh[4],
                         "size": sh[5], "link": sh[6], "entry_size": sh[9]})
    symbols = []
    for section in sections:
        if section["type"] != 2:  # SHT_SYMTAB, not the smaller dynamic table
            continue
        table = sections[section["link"]]
        names = data[table["offset"]:table["offset"] + table["size"]]
        start, end = section["offset"], section["offset"] + section["size"]
        if section["entry_size"] != SYM.size:
            raise ValueError("unexpected symbol entry size")
        for at in range(start, end, SYM.size):
            name, value, size, info, other, shndx = SYM.unpack_from(data, at)
            if name and shndx < len(sections):
                symbols.append({"name": cstr(names, name), "address": value,
                                "size": size, "type": info & 15, "binding": info >> 4,
                                "section": sections[shndx]["name"]})
    text = next(s for s in sections if s["name"] == ".text")
    funcs = sorted([s for s in symbols if s["type"] == 2 and s["section"] == ".text"],
                   key=lambda s: (s["address"], s["name"]))
    comments = next(s for s in sections if s["name"] == ".comment")
    compiler_tags = [x.decode("ascii", "replace") for x in
                     data[comments["offset"]:comments["offset"] + comments["size"]].split(b"\0")
                     if x.startswith(b"GCC:")]
    compilers = sorted(set(compiler_tags))
    compiler_counts = {tag: compiler_tags.count(tag) for tag in compilers}
    return data, {"target": str(ARCHIVE.relative_to(ROOT)) + "::" + MEMBER,
                  "sha256": digest, "file_size": len(data),
                  "elf": {"class": "ELF32", "machine": "i386", "entry": header[4]},
                  "text": {"address": text["address"], "size": text["size"]},
                  "compilers": compilers, "compiler_comment_counts": compiler_counts,
                  "sections": sections,
                  "symbols": symbols, "functions": funcs}


def write_baseline(info):
    CONFIG.mkdir(parents=True, exist_ok=True)
    public = {key: info[key] for key in ("target", "sha256", "file_size", "elf", "text", "compilers")}
    public["section_count"] = len(info["sections"])
    public["symbol_count"] = len(info["symbols"])
    public["function_symbol_count"] = len(info["functions"])
    public["unique_function_addresses"] = len({f["address"] for f in info["functions"]})
    public["compiler_comment_counts"] = info["compiler_comment_counts"]
    (CONFIG / "target.json").write_text(json.dumps(public, indent=2) + "\n")
    with (CONFIG / "functions.tsv").open("w") as stream:
        stream.write("address\tsize\tbinding\tname\n")
        for f in info["functions"]:
            stream.write("0x%08x\t%d\t%d\t%s\n" %
                         (f["address"], f["size"], f["binding"], f["name"]))
    with (CONFIG / "sections.tsv").open("w") as stream:
        stream.write("index\tname\ttype\tflags\taddress\toffset\tsize\n")
        for s in info["sections"]:
            stream.write("%d\t%s\t%d\t0x%x\t0x%08x\t%d\t%d\n" %
                         (s["index"], s["name"], s["type"], s["flags"],
                          s["address"], s["offset"], s["size"]))


def find_function(info, key):
    for function in info["functions"]:
        if key == function["name"] or key.lower() == "0x%08x" % function["address"]:
            return function
    try:
        addr = int(key, 0)
    except ValueError:
        raise ValueError("unknown function: " + key)
    matches = [f for f in info["functions"] if f["size"] and f["address"] <= addr < f["address"] + f["size"]]
    if not matches:
        raise ValueError("no sized function contains address 0x%x" % addr)
    return min(matches, key=lambda f: f["size"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    subs = parser.add_subparsers(dest="command", required=True)
    inv = subs.add_parser("inventory", help="display canonical ELF inventory")
    inv.add_argument("--json", action="store_true")
    inv.add_argument("--write", action="store_true", help="regenerate tracked baseline files")
    subs.add_parser("status", help="report available baseline, not a matching score")
    for command in ("info", "asm"):
        sub = subs.add_parser(command)
        sub.add_argument("function", help="ELF function symbol or hexadecimal address")
    args = parser.parse_args()
    data, info = load()
    if args.command == "inventory":
        if args.write:
            write_baseline(info)
        if args.json:
            print(json.dumps(info, indent=2))
        else:
            print("VUC %s, sha256 %s" % (info["file_size"], info["sha256"]))
            print("%d sections, %d symbols, %d text function symbols" %
                  (len(info["sections"]), len(info["symbols"]), len(info["functions"])))
            print("Text: 0x%08x (%d bytes)" % (info["text"]["address"], info["text"]["size"]))
            for compiler in info["compilers"]:
                print("Compiler: " + compiler)
    elif args.command == "status":
        print("VUC target verified: %d bytes, %d text function symbols" %
              (len(data), len(info["functions"])))
        print("Matching score: UNAVAILABLE (historical compiler and comparator not yet configured)")
    else:
        f = find_function(info, args.function)
        if args.command == "info":
            print("%s: 0x%08x, %d bytes, binding=%d" %
                  (f["name"], f["address"], f["size"], f["binding"]))
        else:
            if not f["size"]:
                raise ValueError("symbol has no size; cannot establish assembly range")
            with tempfile.TemporaryDirectory(prefix="revcl-vuc-") as work:
                path = Path(work) / "vuc"
                path.write_bytes(data)
                subprocess.run(["objdump", "-d", "-C", "--start-address=0x%x" % f["address"],
                                "--stop-address=0x%x" % (f["address"] + f["size"]), str(path)],
                               check=True)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (ValueError, OSError, zipfile.BadZipFile, subprocess.CalledProcessError) as exc:
        print("vuc: " + str(exc), file=sys.stderr)
        sys.exit(1)
