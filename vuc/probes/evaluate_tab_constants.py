#!/usr/bin/env python3
"""Compare normal C++ loop and expression forms for constant-tab scanning."""
import importlib.util
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SCRATCH = Path("/tmp/vuc-binutils215/tab-constants-variants")
COMPILER = "/tmp/vuc-binutils215/g++-jan25-as215"
SOURCE = ROOT / "vuc/probes/asm_output_tab_constants.cpp"
MANIFEST = ROOT / "vuc/config/asm_output_tab_constants_probe.json"

spec = importlib.util.spec_from_file_location("probe", ROOT / "vuc/scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)

baseline = SOURCE.read_text()
rewrites = {
    "baseline": (),
    "vector-alias": (("int count = list.instructions_.size();",
                      "std::vector<Instruction *> &instructions = list.instructions_;\n"
                      "    int count = instructions.size();"),
                     ("list.instructions_[i]", "instructions[i]")),
    "entry-local": (("const char *text = list.instructions_[i]->ToString();",
                     "Instruction *instruction = list.instructions_[i];\n"
                     "        const char *text = instruction->ToString();"),),
    "text-local": (("const char *text = list.instructions_[i]->ToString();",
                    "const char *text;\n"
                    "        text = list.instructions_[i]->ToString();"),),
    "find-assignment": (("const char *delimiter = std::strstr(text, \".assign\");",
                          "const char *delimiter;\n"
                          "        delimiter = std::strstr(text, \".assign\");"),),
    "int-pointer-diff": (("int width = delimiter - text;",
                          "int width = (int)(delimiter - text);"),),
    "signed-index": (("for (int i = 0; i < count; ++i)",
                      "for (signed int i = 0; i < count; ++i)"),),
    "mutable-text": (("const char *ToString();", "char *ToString();"),
                     ("const char *text =", "char *text =")),
    "recheck-count": (("i < count", "i < (int)list.instructions_.size()"),),
    "reverse-maximum": (("width > constantsTabStop", "constantsTabStop < width"),),
    "inline-width": (("int width = delimiter - text;\n            if (width > constantsTabStop)\n"
                      "                constantsTabStop = width;",
                      "if (delimiter - text > constantsTabStop)\n"
                      "                constantsTabStop = delimiter - text;"),),
    "conditional-search": (("const char *delimiter = std::strstr(text, \".assign\");\n"
                            "        if (!delimiter)\n"
                            "            delimiter = std::strstr(text, \".equ\");",
                            "const char *delimiter = std::strstr(text, \".assign\") ?\n"
                            "            std::strstr(text, \".assign\") : std::strstr(text, \".equ\");"),),
    "explicit-index": (("int count = list.instructions_.size();\n    for (int i = 0; i < count; ++i)",
                        "int count = list.instructions_.size();\n    int i = 0;\n"
                        "    for (; i < count; i = i + 1)"),),
    "while-index": (("for (int i = 0; i < count; ++i) {",
                     "int i = 0;\n    while (i < count) {"),
                    ("        }\n    }\n}", "        }\n        ++i;\n    }\n}")),
    "no-local-count": (("int count = list.instructions_.size();\n    ", ""),
                       ("i < count", "i < (int)list.instructions_.size()")),
}

flags = {
    "default": ["-O2", "-frename-registers"],
    "plain": ["-O2"],
    "plain-jump4": ["-O2", "-falign-jumps=4"],
    "jump1": ["-O2", "-frename-registers", "-falign-jumps=1"],
    "jump2": ["-O2", "-frename-registers", "-falign-jumps=2"],
    "jump4": ["-O2", "-frename-registers", "-falign-jumps=4"],
    "loop1": ["-O2", "-frename-registers", "-falign-loops=1"],
    "labels1": ["-O2", "-frename-registers", "-falign-labels=1"],
    "no-regmove": ["-O2", "-falign-jumps=4", "-fno-regmove"],
    "no-sched2": ["-O2", "-falign-jumps=4", "-fno-schedule-insns2"],
    "web": ["-O2", "-falign-jumps=4", "-fweb"],
    "no-cse": ["-O2", "-falign-jumps=4", "-fno-cse-follow-jumps"],
    "no-peephole2": ["-O2", "-falign-jumps=4", "-fno-peephole2"],
    "no-reorder": ["-O2", "-falign-jumps=4", "-fno-reorder-blocks"],
    "no-ifconvert": ["-O2", "-falign-jumps=4", "-fno-if-conversion"],
    "plain-march-i686": ["-O2", "-falign-jumps=4", "-march=i686"],
    "ren-march-i686": ["-O2", "-frename-registers", "-falign-jumps=4", "-march=i686"],
}

SCRATCH.mkdir(parents=True, exist_ok=True)
results = []
for label, replacements in rewrites.items():
    source = baseline
    for old, new in replacements:
        if old not in source:
            raise ValueError("source rewrite did not match: " + label)
        source = source.replace(old, new)
    candidate = SCRATCH / (label + ".cpp")
    candidate.write_text(source)
    for flag_name, options in flags.items():
        obj = SCRATCH / (label + "-" + flag_name + ".o")
        run = subprocess.run([COMPILER, *options, "-c", str(candidate), "-o", str(obj)],
                             capture_output=True, text=True)
        if run.returncode:
            results.append((0, label, flag_name, run.stderr[-100:]))
            continue
        try:
            match = matcher.run(obj, MANIFEST)
            results.append((match["exact_bytes"], label, flag_name,
                            str(match["length"])))
            if match["exact"]:
                print("EXACT", label, flag_name, flush=True)
        except ValueError as exc:
            results.append((0, label, flag_name, str(exc)))

for result in sorted(results, reverse=True)[:25]:
    print(*result)
