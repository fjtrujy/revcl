#!/usr/bin/env python3
"""Recover IsAccumulatorable branch topology with original 11/13/12 order."""
import importlib.util
from pathlib import Path
import subprocess
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/accumulator-branches-oct13');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
head='namespace IR { struct Operand { int tag; int kind; }; struct Operator_Data { int tag; int kind; Operand *operand; }; }\n'
cond='op->operand->kind == 3'
forms={
 'nested':'''if(op->kind==11) { if(C) return true; }
            if(op->kind==13) { if(C) return true; }
            if(op->kind==12) { if(C) return true; }
            return false;''',
 'and':'''if(op->kind==11 && C) return true;
            if(op->kind==13 && C) return true;
            if(op->kind==12 && C) return true;
            return false;''',
 'nested-false':'''if(op->kind==11) {if(C) return true;}
            else if(op->kind==13) {if(C) return true;}
            else if(op->kind==12) {if(C) return true;}
            return false;''',
 'if-not':'''if(op->kind==11) {if(!(C)) goto next13; return true;}
            next13: if(op->kind==13) {if(!(C)) goto next12; return true;}
            next12: if(op->kind==12) {if(!(C)) return false;return true;}
            return false;''',
 'value-first':'''if(op->kind==11) {return C;}
            if(op->kind==13) {return C;}
            if(op->kind==12) {return C;}
            return false;''',
 'bool-flag':'''bool accumulator=false;
             if(op->kind==11 && C) accumulator=true;
             if(op->kind==13 && C) accumulator=true;
             if(op->kind==12 && C) accumulator=true;
             return accumulator;''',
 'switch':'''switch(op->kind) {
               case 11: if(C) return true; break;
               case 13: if(C) return true; break;
               case 12: if(C) return true; break;
              } return false;''',
 'switch-return':'''switch(op->kind) {
               case 11: return C;
               case 13: return C;
               case 12: return C;
              } return false;''',
}
fl={'r':['-O2','-frename-registers'],'j1':['-O2','-frename-registers','-falign-jumps=1'], 'j4':['-O2','-frename-registers','-falign-jumps=4'],'j8':['-O2','-frename-registers','-falign-jumps=8'],'p':['-O2']}
results=[]
for name,body in forms.items():
 p=T/(name+'.cpp');p.write_text(head+'bool IsAccumulatorable(IR::Operator_Data *op) {\n'+body.replace('C',cond)+'\n}\n')
 for kind,opts in fl.items():
  obj=T/(name+'-'+kind+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(p),'-o',str(obj)],capture_output=True,text=True)
  if cp.returncode:print('COMPILE_ERROR',name,kind,cp.stderr[-150:],flush=True);continue
  sym=next((ln for ln in subprocess.run(['nm','-S',str(obj)],capture_output=True,text=True).stdout.splitlines() if '_Z17IsAccumulatorable' in ln),'')
  size=int(sym.split()[1],16) if sym else 0
  if size==77:
   try:
    r=m.run(obj,R/'vuc/config/ir_accumulatorable_probe.json')
    results.append((r['exact_bytes'],size,name,kind))
    print(name,kind,r['exact_bytes'],size,flush=True)
    if r['exact']: print('EXACT',p,kind,flush=True); raise SystemExit(0)
   except ValueError as e:print('match-error',name,kind,str(e)[:150],flush=True)
  else:results.append((0,size,name,kind))
 print('tried',name,flush=True)
for row in sorted(results,reverse=True)[:18]:print('BEST',row)
