#!/usr/bin/env python3
"""Natural scanner allocation-expression controls against the original VUC bytes.

The historical compiler's different liveness around the Flex realloc loop can
change the branching/register allocation. Retain the original function and real
three string relocations; no object patching, assembly, or dummy predecessor.
"""
from pathlib import Path
import subprocess,importlib.util,itertools
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/yy-next-ownership-forms-oct13")
T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("matcher",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
src=(R/"vuc/probes/yy_get_next_buffer_partial_oct13.cpp").read_text()
base={
 "current_offset": "int current_offset = yy_c_buf_p - b->yy_ch_buf;",
 "redirect": "yy_c_buf_p = &b->yy_ch_buf[current_offset];",
 "num_to_read": "num_to_read = b->yy_buf_size - number_to_move - 1;",
 "initial_to_read": "int num_to_read = yy_current_buffer->yy_buf_size - number_to_move - 1;",
 "growth":"if (2 * b->yy_buf_size <= 0)\n                    b->yy_buf_size += ((unsigned)b->yy_buf_size >> 3);\n                else\n                    b->yy_buf_size *= 2;",
 "error": 'if (!b->yy_ch_buf)\n                LexerError("fatal error - scanner input buffer overflow");',
 "saved":'char *old_buf = b->yy_ch_buf;',
 "realloc": 'b->yy_ch_buf = (char*) std::realloc(old_buf, b->yy_buf_size + 2);',
}
for old in base.values():assert old in src,repr(old)
alternates={
 "current_offset":[
 "int current_offset = yy_c_buf_p - b->yy_ch_buf;",
 "int current_offset = yy_c_buf_p - yy_current_buffer->yy_ch_buf;",
 "unsigned int current_offset = yy_c_buf_p - b->yy_ch_buf;",
 "long current_offset = yy_c_buf_p - b->yy_ch_buf;",
 "char *current_pos = yy_c_buf_p; int current_offset = current_pos - b->yy_ch_buf;",
 ],
 "redirect":[
 "yy_c_buf_p = &b->yy_ch_buf[current_offset];",
 "yy_c_buf_p = b->yy_ch_buf + current_offset;",
 "yy_c_buf_p = current_offset + b->yy_ch_buf;",
 "yy_c_buf_p = &yy_current_buffer->yy_ch_buf[current_offset];",
 ],
 "num_to_read":[
 "num_to_read = b->yy_buf_size - number_to_move - 1;",
 "num_to_read = b->yy_buf_size - (number_to_move + 1);",
 "num_to_read = (b->yy_buf_size - 1) - number_to_move;",
 "num_to_read = yy_current_buffer->yy_buf_size - number_to_move - 1;",
 ],
 "growth":[
 base["growth"],
 "int new_size = b->yy_buf_size * 2;\n                if (new_size <= 0)\n                    b->yy_buf_size += ((unsigned)b->yy_buf_size >> 3);\n                else\n                    b->yy_buf_size = new_size;",
 "int new_size = b->yy_buf_size * 2;\n                if (new_size > 0) b->yy_buf_size = new_size;\n                else b->yy_buf_size += ((unsigned)b->yy_buf_size >> 3);",
 "if (b->yy_buf_size * 2 > 0) b->yy_buf_size = 2*b->yy_buf_size;\n                else b->yy_buf_size += ((unsigned)b->yy_buf_size >> 3);",
 ],
 "error":[
 base["error"],
 'if (b->yy_ch_buf == 0)\n                LexerError("fatal error - scanner input buffer overflow");',
 'if (b->yy_ch_buf != 0) {} else\n                LexerError("fatal error - scanner input buffer overflow");',
 ],
 "saved":[
 base["saved"],
 "char *old_buf = yy_current_buffer->yy_ch_buf;",
 ],
 "realloc":[
 base["realloc"],
 "b->yy_ch_buf = reinterpret_cast<char *>(std::realloc(old_buf, b->yy_buf_size + 2));",
 "char *new_buf = (char*) std::realloc(old_buf, b->yy_buf_size + 2); b->yy_ch_buf = new_buf;",
 ],
}
flags=[
 ("j1",["-O2","-frename-registers","-falign-jumps=1"]),
 ("j2",["-O2","-frename-registers","-falign-jumps=2"])
]
score=[]
for var,alts in alternates.items():
 for i,form in enumerate(alts):
  code=src.replace(base[var],form)
  p=T/(var+"-"+str(i)+".cpp");p.write_text(code)
  for f,opts in flags:
   obj=T/(p.stem+"-"+f+".o")
   build=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,"-c",str(p),"-o",str(obj)],capture_output=True,text=True)
   if build.returncode:
    print("ERROR",p.name,f,build.stderr[-115:],flush=True);continue
   nm=subprocess.check_output(["nm","-S",str(obj)],text=True)
   row=next((r for r in nm.splitlines() if r.endswith("_ZN11yyFlexLexer18yy_get_next_bufferEv")),"")
   size=int(row.split()[1],16) if row else -1
   if size!=550:
    score.append((0,size,p.stem,f));continue
   try:
    r=m.run(obj,R/"vuc/config/yy_get_next_buffer_oct13_probe.json")
    score.append((r["exact_bytes"],size,p.stem,f))
    if r["exact"]:
     print("NEW_EXACT",p,f,flush=True)
     raise SystemExit(0)
   except ValueError as e:print("REJECT",p.name,f,str(e)[:120],flush=True)
 print("DONE",var,flush=True)
print("BEST")
for row in sorted(score,reverse=True)[:25]:print(row)
