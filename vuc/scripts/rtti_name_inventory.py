#!/usr/bin/env python3
"""Reconstruct original VUC C++ RTTI type-name symbols from named ELF metadata.

Read original source-level C++ type *names* from the authenticated ZIP-backed
ELF symbol table. Ordinary C++ declarations and typeid expressions make the
historical compiler emit the corresponding Itanium-ABI _ZTS name symbols.
This is data-only verification, not class-layout or vtable reconstruction.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location("decomp",ROOT/"vuc/scripts/decomp.py")
decomp=importlib.util.module_from_spec(spec);spec.loader.exec_module(decomp)
IR_RE=re.compile(r"_ZTSN2IR([1-9][0-9]*)([A-Za-z_][A-Za-z_0-9]*)E$")
TOP_RE=re.compile(r"_ZTS([1-9][0-9]*)([A-Za-z_][A-Za-z_0-9]*)$")
PREFIX="vuc/config/data_rtti_"
SOURCES={"IR":"vuc/src/IRRttiNames.cpp","top":"vuc/src/ASTRttiNames.cpp"}


def original_type_names(raw, target):
    sections=target["sections"]
    sym=next(x for x in sections if x["name"]==".symtab")
    string_section=sections[sym["link"]]
    strings=raw[string_section["offset"]:string_section["offset"]+string_section["size"]]
    found=[]
    for at in range(sym["offset"],sym["offset"]+sym["size"],decomp.SYM.size):
        name_offset,address,size,flags,_,section_index=decomp.SYM.unpack_from(raw,at)
        name=decomp.cstr(strings,name_offset)
        if not name.startswith("_ZTS") or not size:
            continue
        if (flags&15)!=1 or section_index>=len(sections):
            continue
        section=sections[section_index]
        if section["name"]!=".rodata":
            continue
        match=IR_RE.fullmatch(name)
        if match:
            group="IR"
        else:
            match=TOP_RE.fullmatch(name)
            if not match:
                raise ValueError("unsupported original C++ RTTI name: "+name)
            group="top"
        number,typename=match.groups()
        if int(number)!=len(typename):
            raise ValueError("C++ name length mismatch: "+name)
        if size!=len(name)-4+1:
            raise ValueError("C++ original RTTI name object size mismatch: "+name)
        offset=section["offset"]+address-section["address"]
        literal=(name[4:]+"\0").encode("ascii")
        if raw[offset:offset+size]!=literal:
            raise ValueError("original RTTI name is not canonical Itanium ABI: "+name)
        found.append({"symbol":name,"address":"0x%08x"%address,"size":size,
                      "group":group,"type":typename})
    found.sort(key=lambda row:row["symbol"])
    if len(found)!=81 or len({row["symbol"] for row in found})!=81:
        raise ValueError("expected exactly 81 unique original C++ RTTI type names")
    return found


def cxx_source(group, records):
    member=[row for row in records if row["group"]==group]
    if not member:
        raise ValueError("empty type-name group")
    lines=[
        "// C++ RTTI type-name fragments verified against original VUC ELF.",
        "// These empty declaration shapes are name-only placeholders.",
        "// Their class fields, inheritance, vtables and object layout remain",
        "// completely unverified and must never be inferred from this file.",
        "#include <typeinfo>",
        "",
    ]
    if group=="IR":
        lines.append("namespace IR {")
    for row in member:
        lines.append("struct %s {};"%row["type"])
    if group=="IR":
        lines.append("}")
    lines.append("")
    for idx,row in enumerate(member):
        qualified=("IR::" if group=="IR" else "")+row["type"]
        lines.append("const std::type_info &vuc_rtti_name_%s_%02d = typeid(%s);"%
                     (group,idx,qualified))
    return "\n".join(lines)+"\n"


def data_config(record):
    return {"symbol":record["symbol"],"address":record["address"],
            "size":record["size"],"section":".rodata",
            "source":SOURCES[record["group"]],"flags":["-O2"],"relocations":{}}


def config_path(record):
    return ROOT/(PREFIX+record["symbol"]+".json")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write",action="store_true",
                        help="write deterministic C++ type declarations and data configs")
    args=parser.parse_args()
    raw,target=decomp.load()
    records=original_type_names(raw,target)
    groups={group:sum(row["group"]==group for row in records)
            for group in SOURCES}
    print("Original VUC ABI RTTI name objects:",len(records),"grouped",groups)
    print("Exact original bytes to check:",sum(row["size"] for row in records))
    if args.write:
        for group,src in SOURCES.items():
            (ROOT/src).write_text(cxx_source(group,records))
        for record in records:
            config_path(record).write_text(
                json.dumps(data_config(record),indent=2)+"\n")
        manifest=ROOT/"vuc/config/exact_data_candidates.json"
        current=json.loads(manifest.read_text())
        new=[str(config_path(row).relative_to(ROOT)) for row in records]
        if len(set(new))!=len(new):
            raise ValueError("duplicate original RTTI names")
        for entry in new:
            if entry not in current:
                current.append(entry)
        manifest.write_text(json.dumps(current,indent=2)+"\n")
        print("Wrote",len(records),"source-backed original RTTI data configs")


if __name__=="__main__":
    main()
