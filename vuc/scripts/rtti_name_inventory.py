#!/usr/bin/env python3
"""Reconstruct VUC C++ RTTI type-name and inheritance objects from original ELF.

Read original source-level C++ type *names* from the authenticated ZIP-backed
ELF symbol table. Ordinary C++ declarations and typeid expressions make the
historical compiler emit the corresponding Itanium-ABI _ZTS name symbols.
This is data-only verification, not complete object layout or vtable reconstruction.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import re
import struct

ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location("decomp",ROOT/"vuc/scripts/decomp.py")
decomp=importlib.util.module_from_spec(spec);spec.loader.exec_module(decomp)
IR_RE=re.compile(r"_ZTSN2IR([1-9][0-9]*)([A-Za-z_][A-Za-z_0-9]*)E$")
TOP_RE=re.compile(r"_ZTS([1-9][0-9]*)([A-Za-z_][A-Za-z_0-9]*)$")
PREFIX="vuc/config/data_rtti_"
TYPEINFO_PREFIX="vuc/config/data_rtti_typeinfo_"
SOURCES={"IR":"vuc/src/IRRttiNames.cpp","top":"vuc/src/ASTRttiNames.cpp"}
ABI_VTABLES={
    8:("_ZTVN10__cxxabiv117__class_type_infoE",
       "_ZTVN10__cxxabiv117__class_type_infoE@@GLIBCPP_3.2", 0x0808aa40),
    12:("_ZTVN10__cxxabiv120__si_class_type_infoE",
        "_ZTVN10__cxxabiv120__si_class_type_infoE@@GLIBCPP_3.2", 0x0808a9e8),
}


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


def original_type_infos(raw, target, type_names):
    """Recover complete original single-inheritance metadata, not invented bases."""
    sections=target["sections"]
    sym=next(x for x in sections if x["name"]==".symtab")
    stringsec=sections[sym["link"]]
    strings=raw[stringsec["offset"]:stringsec["offset"]+stringsec["size"]]
    original={}
    for at in range(sym["offset"],sym["offset"]+sym["size"],decomp.SYM.size):
        no,addr,size,flags,_,index=decomp.SYM.unpack_from(raw,at)
        name=decomp.cstr(strings,no)
        if name.startswith("_ZTI") or any(name==item[1] for item in ABI_VTABLES.values()):
            if name in original:
                raise ValueError("duplicate original type-info/ABI symbol: "+name)
            original[name]=(addr,size,flags,index)
    for _,original_name,address in ABI_VTABLES.values():
        entry=original.get(original_name)
        if not entry or entry[0]!=address or (entry[2]&15)!=1:
            raise ValueError("original ABI typeinfo vtable is not authenticated: "+
                             original_name)
    names_by_zti={"_ZTI"+n["symbol"][4:]:n for n in type_names}
    by_address={}
    for symbol,n in names_by_zti.items():
        orig=original.get(symbol)
        if not orig or orig[1] not in ABI_VTABLES or (orig[2]&15)!=1:
            raise ValueError("original typeinfo missing or wrong kind/size: "+symbol)
        if orig[3]>=len(sections) or sections[orig[3]]["name"]!=".rodata":
            raise ValueError("original typeinfo outside .rodata: "+symbol)
        if orig[0] in by_address:
            raise ValueError("duplicate original typeinfo address")
        by_address[orig[0]]=symbol
    result=[]
    for symbol,n in sorted(names_by_zti.items()):
        address,size,_,idx=original[symbol]
        section=sections[idx]
        start=section["offset"]+address-section["address"]
        words=struct.unpack_from("<"+"I"*(size//4),raw,start)
        first=ABI_VTABLES[size][2]+8
        if words[0]!=first or words[1]!=int(n["address"],0):
            raise ValueError("original RTTI ABI vtable/type-name reference mismatch: "+
                             symbol)
        parent_symbol=None
        if size==12:
            parent_symbol=by_address.get(words[2])
            if parent_symbol is None:
                raise ValueError("unrecognized original RTTI parent at "+hex(address))
            if names_by_zti[parent_symbol]["group"]!=n["group"]:
                raise ValueError("cross-unit class inheritance must be investigated")
        result.append({"symbol":symbol,"address":"0x%08x"%address,
                       "size":size,"group":n["group"],"type":n["type"],
                       "parent_symbol":parent_symbol})
    if len(result)!=81 or sum(x["size"]==12 for x in result)!=72:
        raise ValueError("expected 81 authentic original typeinfo records, 72 derived")
    return result


def cxx_source(group, records, type_infos=None):
    member=[row for row in records if row["group"]==group]
    if not member:
        raise ValueError("empty type-name group")
    by_typeinfo={r["symbol"]:r for r in (type_infos or [])}
    by_class={r["symbol"]:r for r in member}
    ordered=[]
    visited=set()
    pending=set()

    def visit(row):
        key=row["symbol"]
        if key in pending:
            raise ValueError("cycle in authenticated C++ inheritance graph")
        if key in visited:
            return
        pending.add(key)
        info=by_typeinfo.get("_ZTI"+key[4:])
        if info is not None and info["parent_symbol"]:
            parent_name="_ZTS"+info["parent_symbol"][4:]
            parent=by_class.get(parent_name)
            if parent is None:
                raise ValueError("parent class declaration absent: "+key)
            visit(parent)
        pending.remove(key)
        visited.add(key)
        ordered.append(row)

    for row in member:
        visit(row)
    lines=[
        "// C++ RTTI type-name and single-base inheritance fragments.",
        "// Class names and single-inheritance links are original ELF evidence.",
        "// Member fields, virtual methods, and runtime object layouts remain",
        "// unverified. These data-only probes emit no reconstructed functions.",
        "#include <typeinfo>",
        "",
    ]
    if group=="IR":
        lines.append("namespace IR {")
    for row in ordered:
        info=by_typeinfo.get("_ZTI"+row["symbol"][4:])
        base=(by_typeinfo[info["parent_symbol"]]["type"]
              if info is not None and info["parent_symbol"] else None)
        if base:
            lines.append("struct %s : public %s {};"%(row["type"],base))
        else:
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


def typeinfo_config(record, types, all_infos):
    source=SOURCES[record["group"]]
    abi_name,abi_original,abi_address=ABI_VTABLES[record["size"]]
    suffix=record["symbol"][4:]
    type_name="_ZTS"+suffix
    name_record=next(t for t in types if t["symbol"]==type_name)
    relocations={
        "0x0":{"symbol":abi_name,"original_symbol":abi_original,
                "address":"0x%08x"%abi_address,"type":1},
        "0x4":{"symbol":type_name,"address":name_record["address"],"type":1},
    }
    if record["parent_symbol"]:
        parent_info=next(t for t in all_infos if t["symbol"]==
                         record["parent_symbol"])
        relocations["0x8"]={"symbol":parent_info["symbol"],
                            "address":parent_info["address"],"type":1}
    return {"symbol":record["symbol"],"address":record["address"],
            "size":record["size"],"section":".rodata","source":source,
            "flags":["-O2"],"relocations":relocations}


def typeinfo_config_path(record):
    return ROOT/(TYPEINFO_PREFIX+record["symbol"]+".json")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write",action="store_true",
                        help="write deterministic C++ type declarations and data configs")
    args=parser.parse_args()
    raw,target=decomp.load()
    records=original_type_names(raw,target)
    typeinfo_records=original_type_infos(raw,target,records)
    groups={group:sum(row["group"]==group for row in records)
            for group in SOURCES}
    print("Original VUC ABI RTTI name objects:",len(records),"grouped",groups)
    print("Exact original bytes to check:",sum(row["size"] for row in records))
    print("Original typeinfo objects:",len(typeinfo_records),"exact original bytes:",
          sum(row["size"] for row in typeinfo_records))
    if args.write:
        for group,src in SOURCES.items():
            (ROOT/src).write_text(cxx_source(group,records,typeinfo_records))
        for record in records:
            config_path(record).write_text(
                json.dumps(data_config(record),indent=2)+"\n")
        for record in typeinfo_records:
            typeinfo_config_path(record).write_text(
                json.dumps(typeinfo_config(record,records,typeinfo_records),
                           indent=2)+"\n")
        manifest=ROOT/"vuc/config/exact_data_candidates.json"
        current=json.loads(manifest.read_text())
        new=([str(config_path(row).relative_to(ROOT)) for row in records]+
             [str(typeinfo_config_path(row).relative_to(ROOT))
              for row in typeinfo_records])
        if len(set(new))!=len(new):
            raise ValueError("duplicate original RTTI names")
        for entry in new:
            if entry not in current:
                current.append(entry)
        manifest.write_text(json.dumps(current,indent=2)+"\n")
        print("Wrote",len(records),"source-backed original RTTI data configs")


if __name__=="__main__":
    main()
