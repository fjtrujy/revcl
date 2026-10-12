#!/usr/bin/env python3
"""Compare ordinary C++ line-comment insertions with original ELF32 function."""
import importlib.util
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TMP = Path("/tmp/vuc-binutils215/addline-oct12")
TMP.mkdir(parents=True, exist_ok=True)
SPEC = importlib.util.spec_from_file_location("matcher", ROOT / "vuc/scripts/match_probe.py")
MATCHER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MATCHER)
CONFIG = ROOT / "vuc/config/instruction_add_line_comment_oct12_probe.json"
COMPILER = "/tmp/vuc-binutils215/g++-jan25-as215"

HEADER = '''#include "{root}/vuc/src/MLGameString.h"
#include <vector>
#include <cstddef>
#include <new>
struct Instruction {{
    MLGameString text;
    int line;
    int kind;
    {ctor}
    static void *operator new(std::size_t);
}};
class InstructionList {{
public:
    void Add(Instruction *instruction);
    void AddLineComment(int line);
    std::vector<Instruction *> instructions_;
}};
'''
COND = '''if (instructions_.size()) {{
 Instruction *last = instructions_[instructions_.size() - 1];
 if (last->kind == 3 && last->line == line) return;
}}'''
CONDITIONS = {
 "indexed": COND,
 "last-index": '''unsigned n = instructions_.size(); if (n && instructions_[n-1]->kind == 3 && instructions_[n-1]->line == line) return;''',
 "nested": '''if (instructions_.size() > 0) {
  if (instructions_[instructions_.size()-1]->kind == 3) {
   if (instructions_[instructions_.size()-1]->line == line) return;
  }
 }''',
}
CTORS = {
 "kind": 'Instruction(const MLGameString &s): text(s), kind(3) {}',
 "both": 'Instruction(const MLGameString &s, int n): text(s), line(n), kind(3) {}',
 "param": 'Instruction(const MLGameString &s, int n, int k): text(s), line(n), kind(k) {}',
 "body": 'Instruction(const MLGameString &s, int n): text(s) { kind = 3; line = n; }',
}
ACTIONS = {
 "kind": '''Instruction *item = new Instruction(MLGameString("", false));
 item->line = line; Add(item);''',
 "both": '''Instruction *item = new Instruction(MLGameString("", false), line);
 Add(item);''',
 "param": '''Instruction *item = new Instruction(MLGameString("", false), line, 3);
 Add(item);''',
 "body": '''Instruction *item = new Instruction(MLGameString("", false), line);
 Add(item);''',
}
FLAGS = {
 "j1": ["-O2", "-frename-registers", "-falign-jumps=1"],
 "j2": ["-O2", "-frename-registers", "-falign-jumps=2"],
 "j4": ["-O2", "-frename-registers", "-falign-jumps=4"],
 "j8": ["-O2", "-frename-registers", "-falign-jumps=8"],
 "j16": ["-O2", "-frename-registers", "-falign-jumps=16"],
}
results = []
for c_name, ctor in CTORS.items():
 for cond_name, cond in CONDITIONS.items():
  source = HEADER.format(root=ROOT, ctor=ctor) + "\nvoid InstructionList::AddLineComment(int line) {\n" + cond + "\n" + ACTIONS[c_name] + "\n}\n"
  path = TMP / (c_name + "-" + cond_name + ".cpp")
  path.write_text(source)
  for tag, flags in FLAGS.items():
   obj = TMP / (c_name + "-" + cond_name + "-" + tag + ".o")
   cp = subprocess.run([COMPILER, *flags, "-c", str(path), "-o", str(obj)], capture_output=True, text=True)
   if cp.returncode:
    results.append((0, c_name, cond_name, tag, "compile error: " + cp.stderr[-100:]))
    continue
   try:
    r = MATCHER.run(obj, CONFIG)
    score = (r["exact_bytes"], c_name, cond_name, tag, str(r["length"]) + "/162")
    results.append(score)
    if r["exact"]:
     print("EXACT", score, flush=True)
     raise SystemExit(0)
   except ValueError as exc:
    results.append((0, c_name, cond_name, tag, str(exc)[:80]))
  print("checked", c_name, cond_name, flush=True)
for row in sorted(results, reverse=True)[:30]:
 print(*row)
