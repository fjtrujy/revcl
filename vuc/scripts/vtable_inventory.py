#!/usr/bin/env python3
"""Data-only research: reconstruct original VUC 12-byte vtables from C++.

The instance declarations merely force historical GCC to emit vtables.
They are NOT authenticated original global variables/translation units.
The authenticated evidence is the inheritance graph, inherited virtual
method symbol, 3 original words and original symbol address.
"""
import importlib.util
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("rtti_inventory", ROOT/"vuc/scripts/rtti_name_inventory.py")
inv = importlib.util.module_from_spec(spec)
spec.loader.exec_module(inv)

SOURCE = {"IR": "vuc/src/IRVtableProbes.cpp", "top": "vuc/src/ASTVtableProbes.cpp"}
VTABLE_CONFIG_PREFIX = "vuc/config/data_vtable_"
BASE_METHODS = {
    "ASTNode": ("_ZN7ASTNode3apaEv", 0x080607b0),
    "Type_Data": ("_ZN2IR9Type_Data16zzzzMakeRTTIWorkEv", 0x08050c20),
    "Operator_Data": ("_ZN2IR13Operator_Data16zzzzMakeRTTIWorkEv", 0x08050c30),
    "Expr_Data": ("_ZN2IR9Expr_Data16zzzzMakeRTTIWorkEv", 0x08050c40),
    "UnOperator_Data": ("_ZN2IR15UnOperator_Data16zzzzMakeRTTIWorkEv", 0x0806e2d0),
    "Stmt_Data": ("_ZN2IR9Stmt_Data16zzzzMakeRTTIWorkEv", 0x0806e2e0),
    "Fragment_Data": ("_ZN2IR13Fragment_Data16zzzzMakeRTTIWorkEv", 0x0806e2f0),
    "Module_Data": ("_ZN2IR11Module_Data16zzzzMakeRTTIWorkEv", 0x0806e300),
}


def inventory(raw=None, target=None):
    if (raw is None) != (target is None):
        raise ValueError("original ELF bytes and symbol inventory must be supplied together")
    if raw is None:
        raw, target = inv.decomp.load()
    names = inv.original_type_names(raw, target)
    info = inv.original_type_infos(raw, target, names)
    info_by_symbol = {record["symbol"]: record for record in info}
    suffix_to_info = {record["symbol"][4:]: record for record in info}
    original_symbols = {(item["name"], item["address"], item["type"], item["section"])
                        for item in target["symbols"]}
    sec = next(s for s in target["sections"] if s["name"] == ".rodata")
    found = []
    for symbol in target["symbols"]:
        if (symbol["type"] != 1 or symbol["section"] != ".rodata"
                or symbol["size"] != 12 or not symbol["name"].startswith("_ZTV")):
            continue
        name = symbol["name"]
        suffix = name[4:]
        if suffix not in suffix_to_info:
            raise ValueError("12-byte vtable missing authenticated RTTI: " + name)
        ident = suffix_to_info[suffix]
        at = sec["offset"] + symbol["address"] - sec["address"]
        top, ptr, virtual = struct.unpack_from("<III", raw, at)
        if top != 0 or ptr != int(ident["address"], 0):
            raise ValueError("unexpected original vtable header at " + hex(symbol["address"]))
        candidate_methods = [x for x in BASE_METHODS.values() if x[1] == virtual]
        if len(candidate_methods) != 1:
            raise ValueError("unrecognized original virtual slot at " + hex(symbol["address"]))
        meth, _ = candidate_methods[0]
        if (meth, virtual, 2, ".text") not in original_symbols:
            raise ValueError("original inherited virtual function symbol absent: " + meth)
        class_name = ident["type"]
        ancestor = ident
        while ancestor["type"] not in BASE_METHODS:
            if not ancestor["parent_symbol"]:
                raise ValueError("virtual method absent from inheritance tree: " + name)
            ancestor = info_by_symbol[ancestor["parent_symbol"]]
        if meth != BASE_METHODS[ancestor["type"]][0]:
            raise ValueError("inherited method mismatches original vtable: " + name)
        found.append({
            "symbol": name, "address": "0x%08x" % symbol["address"],
            "size": 12, "section": ".rodata", "source": SOURCE[ident["group"]],
            "flags": ["-O2"], "relocations": {
                "0x04": {"symbol": ident["symbol"], "address": ident["address"], "type": 1},
                "0x08": {"symbol": meth, "address": "0x%08x" % virtual, "type": 2}
            }, "group": ident["group"], "type": class_name
        })
    found.sort(key=lambda item: item["symbol"])
    if len(found) != 67:
        raise ValueError("expected 67 original twelve-byte virtual tables")
    return names, info, found


def vtable_source(group, names, info, vtables):
    types = [r for r in names if r["group"] == group]
    info_by = {r["symbol"]: r for r in info}
    type_by = {r["symbol"]: r for r in types}
    emitted, active, order = set(), set(), []

    def visit(typ):
        key = typ["symbol"]
        if key in emitted:
            return
        if key in active:
            raise ValueError("inheritance cycle")
        active.add(key)
        entry = info_by["_ZTI" + key[4:]]
        if entry["parent_symbol"]:
            parent = type_by["_ZTS" + entry["parent_symbol"][4:]]
            visit(parent)
        active.remove(key)
        emitted.add(key)
        order.append(typ)

    for typ in types:
        visit(typ)
    lines = [
        "// DATA-ONLY compiler emission probe for original C++ vtables.",
        "// Synthetic static instances are NOT original VUC global objects.",
        "// Exact acceptance applies only to named 12-byte original vtables.",
        "// Each inheritance edge and virtual method is verified from original ELF.",
        ""
    ]
    if group == "IR":
        lines.append("namespace IR {")
    for typ in order:
        info_row = info_by["_ZTI" + typ["symbol"][4:]]
        base = (info_by[info_row["parent_symbol"]]["type"] if info_row["parent_symbol"] else None)
        if typ["type"] in BASE_METHODS and (group == "IR" or typ["type"] == "ASTNode"):
            method = "zzzzMakeRTTIWork" if group == "IR" else "apa"
            addition = " virtual void " + method + "();"
        else:
            addition = ""
        inheritance = (" : public " + base) if base else ""
        lines.append("struct %s%s {%s };" % (typ["type"], inheritance, addition))
    if group == "IR":
        lines.append("}")
    lines.append("")
    supported = {x["type"] for x in vtables if x["group"] == group}
    for name in sorted(supported):
        qualified = ("IR::" if group == "IR" else "") + name
        lines.append("static %s vuc_vtable_emission_%s __attribute__((used));" % (qualified, name))
    return "\n".join(lines) + "\n"


def config_path(item):
    return ROOT/(VTABLE_CONFIG_PREFIX + item["symbol"] + ".json")


def main():
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true")
    args = parser.parse_args()
    names, types, vtables = inventory()
    print("Original 12-byte class vtables:", len(vtables),
          "by family", {k: sum(x["group"] == k for x in vtables) for k in SOURCE})
    if args.write:
        for group, path in SOURCE.items():
            (ROOT/path).write_text(vtable_source(group, names, types, vtables))
        for item in vtables:
            record = {k: v for k, v in item.items() if k not in ("group", "type")}
            config_path(item).write_text(json.dumps(record, indent=2) + "\n")
        print("Generated", len(vtables), "data configs and", len(SOURCE), "source fragments")


if __name__ == "__main__":
    main()
