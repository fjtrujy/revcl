#!/usr/bin/env python3
"""VCL-style inline real predecessors for VUC Instruction arena dead-pop.

The original Instruction cleanup is 45/46: the only unmatched byte is its
peephole2-dead pop register, original ECX rather than GCC's EBX/EAX. Unlike
changing the destructor directly, this tests genuinely consumed inline helpers
in earlier accepted InstructionList/Instruction methods. Helpers have no asm,
dummy effects, hard-register binding, or output byte patches.

Both the 46-byte cleanup and concurrently original-exact 265-byte STL vector
sibling must match; accepted method regressions are checked on candidates.
"""
from pathlib import Path
import importlib.util, json, subprocess
V=Path(__file__).resolve().parents[1]
O=Path("/tmp/vuc-instruction-predecessor-inline-20261123");O.mkdir(exist_ok=True)
unit=(V/"src/InstructionListLineCommentUnit.cpp").read_text()
unit=unit.replace('#include "MLGameString.h"','#include "'+str(V/"src/MLGameString.h")+'"')
unit="#include <iostream>\n#include <new>\n"+unit
unit=unit.replace("    static void *operator new(std::size_t);",
                 "    static char *instrData;\n    static void *operator new(std::size_t);",1)
arena=(V/"probes/instruction_full_source_unit_oct13.cpp").read_text()
arena=arena[arena.index("char *Instruction::instrData;"):]
arena=arena.replace("return text_.c_str();","return text.c_str();")
full=unit+"\n"+arena
vec="""
template void std::vector<Instruction *>::_M_insert_aux(
    std::vector<Instruction *>::iterator, Instruction *const &);
"""
forms={
 "baseline": ("","", ""),
 "add-push":(
 "static inline void addToList(std::vector<Instruction*>& list, Instruction *p) { list.push_back(p); }\n",
 "    instructions_.push_back(instruction);",
 "    addToList(instructions_, instruction);"),
 "add-push-reference":(
 "static inline void addToList(std::vector<Instruction*>& list, Instruction *const &p) { list.push_back(p); }\n",
 "    instructions_.push_back(instruction);",
 "    addToList(instructions_, instruction);"),
 "add-push-template":(
 "template<typename T> static inline void addToList(std::vector<T> &list, const T &p) { list.push_back(p); }\n",
 "    instructions_.push_back(instruction);",
 "    addToList(instructions_, instruction);"),
 "print-line":(
 "static inline void printInstructionLine(std::ostream &out, Instruction *p) { out << p->ToString() << std::endl; }\n",
 "        output << (*it)->ToString() << std::endl;",
 "        printInstructionLine(output, *it);"),
 "print-line-reverse":(
 "static inline void printInstructionLine(Instruction *p, std::ostream &out) { out << p->ToString() << std::endl; }\n",
 "        output << (*it)->ToString() << std::endl;",
 "        printInstructionLine(*it, output);"),
 "print-get-text":(
 "static inline const char *textOfInstruction(Instruction *p) { return p->ToString(); }\n",
 "        output << (*it)->ToString() << std::endl;",
 "        output << textOfInstruction(*it) << std::endl;"),
 "addl-create":(
 "static inline Instruction *newInstruction(const MLGameString &name) { return new Instruction(name); }\n",
 "    Add(new Instruction(name));",
 "    Add(newInstruction(name));"),
 "line-get-previous":(
 "static inline Instruction *lastInstruction(std::vector<Instruction*> &v) { return v[v.size()-1]; }\n",
 "        Instruction *previous = instructions_[instructions_.size() - 1];",
 "        Instruction *previous = lastInstruction(instructions_);"),
 "line-check-previous":(
 "static inline bool sameLineComment(Instruction *p, int line) { return __builtin_expect(p->kind == 3, 0) && p->reserved == line; }\n",
 "        if (__builtin_expect(previous->kind == 3, 0) && previous->reserved == line)",
 "        if (sameLineComment(previous, line))"),
 "line-new-empty":(
 'static inline Instruction *newEmptyInstruction() { return new Instruction(MLGameString("", false)); }\n',
 '    Instruction *instruction = new Instruction(MLGameString("", false));',
 '    Instruction *instruction = newEmptyInstruction();'),
 "format-vsprintf":(
 "static inline int formatInstruction(char *dest, const char *fmt, va_list args) { return vsprintf(dest, fmt, args); }\n",
 "    vsprintf(line, format, arguments);",
 "    formatInstruction(line, format, arguments);"),
 "toString-text":(
 "static inline const char *getText(const MLGameString &text) { return text.c_str(); }\n",
 "    return text.c_str();",
 "    return getText(text);"),
 "allocator-needs-chunk":(
 "static inline bool needsChunk(char *base, std::size_t size) { return *(char**)base + size > base + 0x2004; }\n",
 "    if (*(char **)instrData + size > instrData + 0x2004) {",
 "    if (needsChunk(instrData, size)) {"),
 "allocator-next":(
 "static inline char *nextChunk(char *chunk) { return chunk + 4; }\n",
 "        *(char **)chunk = chunk + 4;",
 "        *(char **)chunk = nextChunk(chunk);"),
 "arena-ctor-size":(
 "static inline char *newInstructionChunk() { return (char *)::operator new(0x2008); }\n",
 "        char *chunk = (char *)::operator new(0x2008);",
 "        char *chunk = newInstructionChunk();"),
 "arena-instrdata-access":(
 "static inline char *&instructionChunk() { return Instruction::instrData; }\n",
 "        Instruction::instrData = chunk;",
 "        instructionChunk() = chunk;"),
}
anchors={
 "add":"void InstructionList::Add(Instruction *instruction)",
 "print":"void InstructionList::Print(std::ostream &output)",
 "addl":"void InstructionList::AddL(const MLGameString &name)",
 "line":"void InstructionList::AddLineComment(int line)",
 "format":"void InstructionList::Add(const char *format, ...)",
 "tostring":"const char *Instruction::ToString()",
 "allocator":"void *Instruction::operator new(std::size_t size)",
 "arena":"struct InstructionArenaCleanup {",
}
groups={
 "add":"add", "print":"print","addl":"addl","line":"line",
 "format":"format","toString":"tostring","allocator":"allocator","arena":"arena",
}
def anchor_for(label):
  if label.startswith("addl-"):return anchors["addl"]
  if label.startswith("add-"):return anchors["add"]
  if label.startswith("print-"):return anchors["print"]
  if label.startswith("line-"):return anchors["line"]
  if label.startswith("format-"):return anchors["format"]
  if label.startswith("toString-"):return anchors["tostring"]
  if label.startswith("allocator-"):return anchors["allocator"]
  if label.startswith("arena-"):return anchors["arena"]
  return ""
spec=importlib.util.spec_from_file_location("m",V/"scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
targets={
 "cleanup":"instruction_arena_cleanup_unit_oct14_probe.json",
 "vector":"instruction_vector_insert_exact_oct15i.json",
 "add":"instruction_list_add_exact_probe.json",
 "addl":"instruction_addl_probe.json",
 "print":"instruction_list_print_probe.json",
 "line":"instruction_add_line_comment_context_oct13_probe.json",
 "tostring":"instruction_to_string_probe.json",
 "allocator":"instruction_arena_operator_new_probe.json",
}
rows=[]
for label,(helper,needle,repl) in forms.items():
  for owner in ("append","vector-before-add"):
    src=full
    if helper:
      anchor=anchor_for(label)
      assert anchor and src.count(anchor)==1, (label,anchor)
      src=src.replace(anchor,helper+"\n"+anchor,1)
      assert src.count(needle)>0,(label,needle)
      src=src.replace(needle,repl,1)
    if owner=="vector-before-add":
      src=src.replace(anchors["add"],vec+anchors["add"],1)
    path=O/(label+"-"+owner+".cpp");path.write_text(src)
    for snap in ("jan25","dec06"):
      obj=O/(label+"-"+owner+"-"+snap+".o")
      proc=subprocess.run(["/tmp/vuc-binutils215/g++-"+snap+"-as215",
            "-O2","-frename-registers","-fuse-cxa-atexit","-falign-jumps=1",
            "-c",str(path),"-o",str(obj)],capture_output=True,text=True)
      row={"case":label,"owner":owner,"compiler":snap}
      if proc.returncode:row["compile_error"]=proc.stderr[-500:]
      else:
        for key in ("cleanup","vector"):
          try:
            r=m.run(obj,V/"config"/targets[key])
            row[key]={"bytes":r["exact_bytes"],"size":r["length"],
                      "exact":r["exact"],"diffs":r["different_offsets"]}
          except ValueError as ex:row[key]={"unverified":str(ex)}
        if row["cleanup"].get("exact"):
          for key in ("add","addl","print","line","tostring"):
            try:
              s=m.run(obj,V/"config"/targets[key])
              row[key]={"bytes":s["exact_bytes"],"size":s["length"],"exact":s["exact"]}
            except ValueError as ex:row[key]={"unverified":str(ex)}
      print(json.dumps(row,sort_keys=True),flush=True);rows.append(row)
(O/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
