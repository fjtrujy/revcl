#!/usr/bin/env python3
"""Inspect the four compiler bytes in VUC's Bind(ASTFunCall) diagnostic.

The candidate reproduces 300/304 code bytes and 23 authentic call/data
relocations but has 16 original-section-unowned literal padding bytes.
This is NOT an authenticated partial and must not enter exact_candidates.json.

Original uses "dec eax; mov edi,[esi+eax*4]" for the current match stack top.
Provisional C++ uses the equivalent "[esi+eax*4-4]" addressing. Try actual
C++03 list/indexing abstractions and local value lifetimes, not ASM, register
bindings, or fake linker padding. Score only original-length code with all
23 relocations and all referenced original literal bytes independently checked.
"""
import importlib.util
import json
from pathlib import Path
import subprocess

VUC = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-bind-stack-index-20261123")
OUT.mkdir(exist_ok=True)
source = (VUC / "probes/BindASTFunCallFreshOct15r.cpp").read_text()
source = source.replace('#include "../src/MLGameString.h"',
                        '#include "' + str(VUC / "src/MLGameString.h") + '"')
source = source.replace('#include "../src/ASTNodeList.h"',
                        '#include "' + str(VUC / "src/ASTNodeList.h") + '"')
old = "matchStack.entries[matchStack.length-1]->line"
assert source.count(old) == 1

# A prefix may introduce consumed local C++03 values, but must not mutate
# the global stack's length/capacity or perform any additional IO.
versions = {
    "baseline": ("", old),
    "parenthesized": ("", "matchStack.entries[(matchStack.length-1)]->line"),
    "left-side-subtract": ("", "matchStack.entries[-1+matchStack.length]->line"),
    "add-negative": ("", "matchStack.entries[matchStack.length+(-1)]->line"),
    "pointer-offset": ("", "(*(matchStack.entries+matchStack.length-1))->line"),
    "pointer-end-minus-one": ("", "(*(matchStack.entries+matchStack.length-1))->line"),
    "pointer-end-index": ("", "(matchStack.entries+matchStack.length)[-1]->line"),
    "local-decrement": ("int currentIndex = matchStack.length;\n    --currentIndex;",
                        "matchStack.entries[currentIndex]->line"),
    "local-subtract": ("int currentIndex = matchStack.length - 1;",
                       "matchStack.entries[currentIndex]->line"),
    "local-predecrement": ("int currentIndex = matchStack.length;",
                          "matchStack.entries[--currentIndex]->line"),
    "local-unsigned-decrement": ("unsigned currentIndex = matchStack.length;\n    --currentIndex;",
                                 "matchStack.entries[currentIndex]->line"),
    "local-unsigned-subtract": ("unsigned currentIndex = matchStack.length - 1;",
                                "matchStack.entries[currentIndex]->line"),
    "local-long-decrement": ("long currentIndex = matchStack.length;\n    --currentIndex;",
                             "matchStack.entries[currentIndex]->line"),
    "local-index-pointer": ("ASTNode **items = matchStack.entries;\n    int currentIndex=matchStack.length-1;",
                             "items[currentIndex]->line"),
    "local-end-pointer": ("ASTNode **last = matchStack.entries + matchStack.length;\n    --last;",
                           "(*last)->line"),
    "local-end-offset": ("ASTNode **last = matchStack.entries + matchStack.length;",
                          "last[-1]->line"),
    "local-element": ("ASTNode *last = matchStack.entries[matchStack.length-1];",
                       "last->line"),
    "local-element-line": ("int lastLine = matchStack.entries[matchStack.length-1]->line;",
                            "lastLine"),
    "array-pointer-deref": ("", "(*(matchStack.entries+(matchStack.length-1)))->line"),
    "array-pointer-offset": ("", "(matchStack.entries[matchStack.length-1])->line"),
    "ptrdiff-index": ("std::ptrdiff_t currentIndex = matchStack.length;\n    --currentIndex;",
                       "matchStack.entries[currentIndex]->line"),
}

matcher_spec = importlib.util.spec_from_file_location("m", VUC / "scripts/match_probe.py")
matcher = importlib.util.module_from_spec(matcher_spec)
matcher_spec.loader.exec_module(matcher)
cfg = json.loads((VUC / "config/bind_ast_fun_call_partial_oct15r.json").read_text())
original_bytes, target = matcher.decomp.load()
text = next(s for s in target["sections"] if s["name"] == ".text")
start_address = int(cfg["address"], 16)
expect = original_bytes[text["offset"] + start_address - text["address"]:
                        text["offset"] + start_address - text["address"] + cfg["size"]]
ro = next(s for s in target["sections"] if s["name"] == ".rodata")
rows = []
for label, (prefix, expression) in versions.items():
    text_source = source.replace(old, expression)
    if prefix:
        text_source = text_source.replace("    std::cout << currFileName;",
                                          "    " + prefix + "\n    std::cout << currFileName;", 1)
    path = OUT / (label + ".cpp")
    path.write_text(text_source)
    for snap in ("jan25", "dec06"):
        row = {"variant": label, "compiler": snap, "strict_acceptance": False}
        obj = OUT / (label + "-" + snap + ".o")
        result = subprocess.run(
            ["/tmp/vuc-binutils215/g++-" + snap + "-as215",
             "-O2", "-frename-registers", "-c", str(path), "-o", str(obj)],
            capture_output=True, text=True)
        if result.returncode:
            row["compile_error"] = result.stderr[-350:]
        else:
            raw = obj.read_bytes()
            try:
                sections, _ = matcher.elf_sections(raw)
                literals = matcher.section_bytes(raw, sections[".rodata"][1])
                for literal in cfg["relocation_literals"][".rodata"]:
                    offset = literal["offset"]
                    n = literal["size"]
                    original_offset = ro["offset"] + int(literal["address"], 16) - ro["address"]
                    if literals[offset:offset+n] != original_bytes[original_offset:original_offset+n]:
                        raise ValueError("referenced literal mismatch at %d" % offset)
                code, relocation_count = matcher.prepare_named_function(
                    raw, cfg["function"], cfg["relocations"], start_address,
                    {}, cfg["relocation_literals"])
                if len(code) != cfg["size"] or relocation_count != 23:
                    raise ValueError("different code length / relocation count (%d / %d)" %
                                     (len(code), relocation_count))
                row.update(code_matched=sum(a == b for a, b in zip(code, expect)),
                           total_code=len(code),
                           differences=[i for i, (a,b) in enumerate(zip(code, expect)) if a!=b],
                           authenticated_relocations=relocation_count,
                           authenticated_referenced_literals=65,
                           unowned_rodata_padding=16)
            except (OSError, ValueError, KeyError) as ex:
                row["unverified"] = str(ex)
        print(json.dumps(row, sort_keys=True), flush=True)
        rows.append(row)
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")
