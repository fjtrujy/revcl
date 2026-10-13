#!/usr/bin/env python3
"""Use typed arena chunk metadata for authentic Instruction cleanup.

The original arena stores a next-chunk pointer at byte offset 0x2004.
The existing partial represented the chunk as char*. Test ordinary
typed C++ structs and iteration conventions for the same linked list.
Avoid inline assembler, machine-byte padding and fabricated dependencies.
"""
from pathlib import Path
import subprocess,importlib.util
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/instruction-typed-arena-oct13");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py");m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
structs={
 "plain":"""struct ArenaChunk {
    char arena[0x2004];
    ArenaChunk *next;
};""",
 "char-payload":"""struct ArenaChunk {
    char data[0x2000]; void *freeCursor;
    ArenaChunk *next;
};""",
 "typed-cursor":"""struct ArenaChunk {
    char data[0x2000]; char *freeCursor;
    ArenaChunk *next;
};""",
 "header-plus-buffer":"""struct ArenaChunk {
    void *freeCursor;
    char data[0x2000];
    ArenaChunk *next;
};"""
}
iterations={
"while":"""        while(Instruction::instrData) {
    ArenaChunk *current=Instruction::instrData;
    ArenaChunk *next=current->next;
    ::operator delete(current);
    Instruction::instrData=next;
 }""",
"for":"""        for(ArenaChunk *current=Instruction::instrData;current; current=Instruction::instrData) {
    ArenaChunk *next=current->next;
    ::operator delete(current);
    Instruction::instrData=next;
 }""",
"do":"""        if(Instruction::instrData) do {
    ArenaChunk *current=Instruction::instrData;
    ArenaChunk *next=current->next;
    ::operator delete(current);
    Instruction::instrData=next;
 } while(Instruction::instrData);""",
"for-next":"""        ArenaChunk *current=Instruction::instrData;
        while(current) {
    ArenaChunk *next=current->next;
    ::operator delete(current);
    Instruction::instrData=next;
    current=next;
 }""",
"delete":"""        while(Instruction::instrData) {
    ArenaChunk *current=Instruction::instrData;
    ArenaChunk *next=current->next;
    delete current;
    Instruction::instrData=next;
 }"""
}
flags={
 "baseline":["-O2","-frename-registers","-fuse-cxa-atexit"],
 "segments":["-O2","-frename-registers","-fuse-cxa-atexit","-ffunction-sections"],
 "plain":["-O2","-fuse-cxa-atexit"],
}
rank=[]
for chunk,decl in structs.items():
 for loop,body in iterations.items():
  text="#include <iostream>\n#include <new>\n"+decl+"""
class Instruction { public: static ArenaChunk *instrData; };
ArenaChunk *Instruction::instrData;
struct ArenaCleanup {
    ArenaCleanup() {
        ArenaChunk *chunk = (ArenaChunk *)::operator new(0x2008);
        chunk->next = 0;
        Instruction::instrData = chunk;
    }
    ~ArenaCleanup() {
"""+body+"""
    }
};
static ArenaCleanup cleanup;
"""
  path=T/(chunk+"-"+loop+".cpp");path.write_text(text)
  for name,options in flags.items():
   obj=T/(path.stem+"-"+name+".o")
   cp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*options,"-c",str(path),"-o",str(obj)],capture_output=True,text=True)
   if cp.returncode:print("ERROR",path.stem,name,cp.stderr[-100:],flush=True);continue
   try:
    res=m.run(obj,R/"vuc/config/instruction_arena_cleanup_probe.json")
    rank.append((res["exact_bytes"],path.stem,name,res["length"]))
    if res["exact"]:
     print("NEW_EXACT",path,name,flush=True)
     raise SystemExit(0)
   except ValueError as exc:rank.append((0,path.stem,name,str(exc)[:80]))
 print("TESTED",chunk,flush=True)
print("TOP")
for row in sorted(rank,reverse=True)[:30]:print(row)
