#!/usr/bin/env python3
"""Reconstruct both 240-byte VUC TranslateFragList instantiations using GCC 3.3.

All generated C++/ELF object files stay in /tmp. The original 12-byte
ASTNodeList ABI, 16-element growth policy, 4-byte element pointers, 0x804
fragment calling contract and two relocation targets are binary-supported.
"""
from pathlib import Path
import subprocess
import importlib.util
import itertools

ROOT = Path(__file__).resolve().parents[2]
SCRATCH = Path("/tmp/vuc-binutils215/translate-frag-list-oct14")
SCRATCH.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("matcher", ROOT / "vuc/scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
base = (ROOT / "vuc/probes/translate_frag_list_oct14.cpp").read_text()
begin = base.index("    void Add(T element) {")
end = base.index("\n};", begin)
add = base[begin:end]
forms = {"baseline": add}
forms["old-before-assign"] = """    void Add(T element) {
        if (length >= capacity) {
            int new_capacity = capacity ? 2 * capacity : 16;
            T *old_entries = entries;
            entries = new T[new_capacity];
            if (old_entries)
                __builtin_memcpy(entries, old_entries, length * sizeof(T));
            capacity = new_capacity;
        }
        entries[length++] = element;
    }"""
forms["new-to-field-first"] = """    void Add(T element) {
        if (length >= capacity) {
            int new_capacity = capacity ? 2 * capacity : 16;
            T *old_entries = entries;
            entries = new T[new_capacity];
            if (old_entries)
                __builtin_memcpy(entries, old_entries, length * sizeof(T));
            capacity = new_capacity;
        }
        entries[length] = element;
        ++length;
    }"""
forms["capacity-before-alloc"] = """    void Add(T element) {
        if (length >= capacity) {
            T *old_entries = entries;
            capacity = capacity ? 2 * capacity : 16;
            entries = new T[capacity];
            if (old_entries)
                __builtin_memcpy(entries, old_entries, length * sizeof(T));
        }
        entries[length++] = element;
    }"""
forms["size-after-alloc"] = """    void Add(T element) {
        if (length >= capacity) {
            int new_capacity = capacity ? 2 * capacity : 16;
            T *old_entries = entries;
            entries = new T[new_capacity];
            capacity = new_capacity;
            if (old_entries)
                __builtin_memcpy(entries, old_entries, length * sizeof(T));
        }
        entries[length++] = element;
    }"""
forms["copy-count-local"] = """    void Add(T element) {
        if (length >= capacity) {
            int new_capacity = capacity ? 2 * capacity : 16;
            T *old_entries = entries;
            const unsigned bytes = length * sizeof(T);
            entries = new T[new_capacity];
            if (old_entries)
                __builtin_memcpy(entries, old_entries, bytes);
            capacity = new_capacity;
        }
        entries[length++] = element;
    }"""
forms["pointer-loop"] = """    void Add(T element) {
        if (length >= capacity) {
            int new_capacity = capacity ? 2 * capacity : 16;
            T *old_entries = entries;
            entries = new T[new_capacity];
            if (old_entries)
                for (int i = 0; i < length; ++i)
                    entries[i] = old_entries[i];
            capacity = new_capacity;
        }
        entries[length++] = element;
    }"""
start = base.index("template<class T>\nASTNodeList<IR::Fragment_Data *> TranslateFragList")
body_end = base.index("\ntemplate ASTNodeList", start)
tfunc = base[start:body_end]
loop_forms = {
    "for": tfunc,
    "while": tfunc.replace(
        "    for (int i = 0; i < input.length; ++i)\n        result.Add(TranslateFrag(input.entries[i]));",
        "    int i = 0;\n    while (i < input.length) {\n"
        "        result.Add(TranslateFrag(input.entries[i]));\n        ++i;\n    }"),
    "pointer-loop": tfunc.replace(
        "    for (int i = 0; i < input.length; ++i)\n        result.Add(TranslateFrag(input.entries[i]));",
        "    ASTDecl **current = (ASTDecl **)input.entries;\n"
        "    ASTDecl **end = current + input.length;\n"
        "    while (current != end) result.Add(TranslateFrag(*current++));"),
}
instantiations = {
    "var": ("ASTVarDecl", "vuc/config/translate_frag_list_var_oct14_probe.json"),
    "decl": ("ASTDecl", "vuc/config/translate_frag_list_decl_oct14_probe.json"),
}
flags = {
    "j1": ["-O2", "-frename-registers", "-falign-jumps=1"],
    "j4": ["-O2", "-frename-registers", "-falign-jumps=4"],
    "j8": ["-O2", "-frename-registers", "-falign-jumps=8"],
    "noreorder": ["-O2", "-frename-registers", "-falign-jumps=1", "-fno-reorder-blocks"],
    "plain": ["-O2", "-falign-jumps=1"],
}
rows = []
for (kind, (type_name, config)), (method, add_body), (loop, template_body), (flag, options) in itertools.product(
        instantiations.items(), forms.items(), loop_forms.items(), flags.items()):
    prefix = base[:begin] + add_body + base[end:start]
    candidate = prefix + template_body + "\n" + (
        "template ASTNodeList<IR::Fragment_Data *> TranslateFragList<%s *>(ASTNodeList<%s *> &);\n" % (type_name, type_name))
    tag = "-".join((kind, method, loop, flag))
    source = SCRATCH / (tag + ".cpp")
    source.write_text(candidate)
    obj = SCRATCH / (tag + ".o")
    cc = subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",
                         *options, "-c", str(source), "-o", str(obj)],
                        text=True, capture_output=True)
    if cc.returncode:
        print("COMPILE_FAIL", tag, cc.stderr[-150:], flush=True)
        continue
    nm = subprocess.check_output(["nm", "-S", str(obj)], text=True)
    symbol = next((l for l in nm.splitlines() if "TranslateFragList" in l), "")
    length = int(symbol.split()[1], 16) if symbol else -1
    if length == 240:
        try:
            result = matcher.run(obj, ROOT / config)
            rows.append((result["exact_bytes"], length, tag, result["different_offsets"][:12]))
            if result["exact"]:
                print("NEW_EXACT", kind, tag, source, flush=True)
                break
        except ValueError as exc:
            rows.append((0, length, tag, "reject " + str(exc)[:100]))
    else:
        rows.append((0, length, tag, "size"))
    if flag == "plain":
        print("CHECKED", kind, method, loop, flush=True)
for kind in instantiations:
    print("TOP", kind, flush=True)
    for row in sorted((r for r in rows if r[2].startswith(kind+"-")), reverse=True)[:22]:
        print(row, flush=True)
