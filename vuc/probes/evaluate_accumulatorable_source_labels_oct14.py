#!/usr/bin/env python3
"""VUC 77-byte IsAccumulatorable: genuine edge/label placement variants.

Original has case 12 aligned at +0x20 with a 4-byte GCC NOP; ordinary
candidate puts its entire 11-byte GCC alignment at the true block.
Explore source-level joins and likely edges, never manual NOPs.
"""
from pathlib import Path
import importlib.util,subprocess,itertools
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/accum-labels-oct14');T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('match',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
head=(R/'vuc/probes/ir_accumulatorable.cpp').read_text().split('bool IsAccumulatorable(',1)[0]
original='op->operand->kind == 3'
cases={
'nested':"""    if (op->kind == 11) { if (C) return true; }
    if (op->kind == 13) { if (C) return true; }
    if (op->kind == 12) { if (C) return true; }
    return false;""",
'goto-true':"""    if (op->kind == 11) { if (C) goto yes; }
    if (op->kind == 13) { if (C) goto yes; }
    if (op->kind == 12) { if (C) goto yes; }
    return false;
yes: return true;""",
'goto-false':"""    if (op->kind == 11) { if (C) return true; else goto next13; }
next13: if (op->kind == 13) { if (C) return true; else goto next12; }
next12: if (op->kind == 12) { if (C) return true; }
    return false;""",
'goto-common':"""    if (op->kind == 11) { if (!(C)) goto second; goto yes; }
second: if (op->kind == 13) { if (!(C)) goto third; goto yes; }
third: if (op->kind == 12) { if (!(C)) goto no; goto yes; }
no: return false;
yes: return true;""",
'goto-no':"""    if (op->kind == 11) { if (!(C)) goto next13; return true; }
next13: if (op->kind == 13) { if (!(C)) goto next12; return true; }
next12: if (op->kind == 12) { if (!(C)) goto no; return true; }
no: return false;""",
'switch-join':"""    switch (op->kind) {
    case 11: if (C) goto yes; break;
    case 13: if (C) goto yes; break;
    case 12: if (C) goto yes; break;
    }
    return false;
yes: return true;""",
'switch-return':"""    switch (op->kind) {
    case 11: if (C) return true; break;
    case 13: if (C) return true; break;
    case 12: if (C) return true; break;
    }
    return false;""",
'switch-subreturn':"""    switch (op->kind) {
    case 11: return C;
    case 13: return C;
    case 12: return C;
    }
    return false;""",
'bool-success':"""    bool success = false;
    if (op->kind == 11) { if (C) success=true; }
    if (op->kind == 13) { if (C) success=true; }
    if (op->kind == 12) { if (C) success=true; }
    return success;""",
'if-else':"""    if (op->kind == 11) { if (C) return true; }
    else if (op->kind == 13) { if (C) return true; }
    else if (op->kind == 12) { if (C) return true; }
    return false;""",
}
def hint(expr,prob):
 if prob=='plain':return expr
 return '__builtin_expect('+expr+', '+('0' if prob=='unlikely' else '1')+')'
flags={
 'r-j1':['-O2','-frename-registers','-falign-jumps=1'],
 'r-j1-label1':['-O2','-frename-registers','-falign-jumps=1','-falign-labels=1'],
 'r-j1-loop4':['-O2','-frename-registers','-falign-jumps=1','-falign-loops=4'],
 'r-j2':['-O2','-frename-registers','-falign-jumps=2'],
 'r-j4':['-O2','-frename-registers','-falign-jumps=4'],
 'r-j8':['-O2','-frename-registers','-falign-jumps=8'],
}
scores=[]
for (name,body),(branch,child) in itertools.product(cases.items(), (('plain','plain'),('plain','likely'),('plain','unlikely'),('likely','plain'),('unlikely','plain'),('likely','unlikely'),('unlikely','likely'))):
 source=body.replace('op->kind == 12', hint('op->kind == 12',branch))
 source=source.replace('C',hint(original,child))
 tag=name+'-'+branch+'-'+child
 cpp=T/(tag+'.cpp');cpp.write_text(head+'bool IsAccumulatorable(IR::Operator_Data *op) {\n'+source+'\n}\n')
 for label,opts in flags.items():
  obj=T/(tag+'-'+label+'.o')
  c=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
  if c.returncode:print('COMPILE_FAIL',tag,label,c.stderr[-95:],flush=True);continue
  sym=next((x for x in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines() if x.endswith('_Z17IsAccumulatorablePN2IR13Operator_DataE')),'')
  size=int(sym.split()[1],16) if sym else -1
  if size!=77:scores.append((0,size,tag,label,'length'));continue
  try:
   s=m.run(obj,R/'vuc/config/ir_accumulatorable_probe.json')
   scores.append((s['exact_bytes'],size,tag,label,s['different_offsets'][:16]))
   if s['exact']:
    print('NEW_EXACT',tag,label,cpp,flush=True);raise SystemExit(0)
  except ValueError as e:print('REJECT',tag,label,str(e)[:85],flush=True)
 print('DONE',tag,flush=True)
print('TOP')
for x in sorted(scores,reverse=True)[:26]:print(x,flush=True)
