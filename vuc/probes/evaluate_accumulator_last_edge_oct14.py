#!/usr/bin/env python3
"""Original 77-byte accumulator classifier: recover case-11 final edge.

The joint prediction sweep reproduced 74/77 authenticated bytes; the last
three differ because a likely-child guard made GCC reverse the final branch.
Try equivalent genuine C++03 guard-arm topology and historical optimizer
controls, never inserting instructions or editing object code.
"""
import itertools,subprocess,importlib.util
from pathlib import Path
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/accum-last-edge-oct14');T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
base=(R/'vuc/probes/ir_accumulatorable.cpp').read_text()
base=base.replace('if (op->kind == 11)','if (__builtin_expect(op->kind == 11, 0))')
base=base.replace('if (op->kind == 13)','if (__builtin_expect(op->kind == 13, 0))')
# Case 12 child is likely false, and case 11 child likely true.
base=base.replace('if (op->operand->kind == 3) return true;', 'if (__builtin_expect(op->operand->kind == 3, 1)) return true;',1)
ix=base.index('if (op->kind == 12)')
base=base[:ix]+base[ix:].replace('if (op->operand->kind == 3) return true;', 'if (__builtin_expect(op->operand->kind == 3, 0)) return true;',1)
orig='if (__builtin_expect(op->operand->kind == 3, 1)) return true;'
assert orig in base
cond='op->operand->kind'
forms={
'base':orig,
'negated-eq':f'if (!__builtin_expect({cond} != 3, 0)) return true;',
'negated-eq-explicit':f'if (__builtin_expect({cond} != 3, 0)) {{ }} else return true;',
'negated-likely':f'if (!__builtin_expect({cond} != 3, 0)) {{ return true; }}',
'negated-bool':f'if (__builtin_expect(({cond} != 3) == false, 1)) return true;',
'logical-not':f'if (__builtin_expect(!({cond} != 3), 1)) return true;',
'condition-ternary':f'if (__builtin_expect({cond} == 3, 1) ? true : false) return true;',
'condition-eqtrue':f'if (__builtin_expect({cond} == 3, 1) == true) return true;',
'condition-nonzero':f'if (__builtin_expect({cond} == 3, 1) != 0) return true;',
'condition-intlocal':f'int equal = ({cond} == 3); if (__builtin_expect(equal, 1)) return true;',
'condition-boollocal':f'bool equal = ({cond} == 3); if (__builtin_expect(equal, 1)) return true;',
'condition-kindlocal':f'int childKind = {cond}; if (__builtin_expect(childKind == 3,1)) return true;',
'condition-local-reversed':f'int childKind = {cond}; if (!__builtin_expect(childKind != 3,0)) return true;',
'condition-direct-branch':f'if (__builtin_expect({cond} == 3, 1)) {{ return true; }} else {{ }}',
'condition-negated-else':f'if (__builtin_expect({cond} != 3, 0)) {{ }} else {{ return true; }}',
'condition-earlygoto':f'if (__builtin_expect({cond} != 3,0)) goto nextCase; return true; nextCase: ;',
'condition-earlygoto-positive':f'if (__builtin_expect({cond} == 3,1)) goto successCase; goto nextCase; successCase: return true; nextCase: ;',
'condition-guard-block':f'if (__builtin_expect({cond} == 3,1)) {{ bool found=true; if(found) return true; }}',
'condition-explicit-bool':f'bool found=__builtin_expect({cond} == 3,1); if(found) return true;',
'condition-and-true':f'if (__builtin_expect({cond} == 3,1) && true) return true;',
'condition-cmp-subtract':f'if (__builtin_expect(({cond}-3)==0,1)) return true;',
'condition-xor':f'if (__builtin_expect(({cond}^3)==0,1)) return true;',
'condition-eq-unsigned':f'if (__builtin_expect((unsigned){cond} == 3u,1)) return true;',
}
opts={
'j8':['-O2','-frename-registers','-falign-jumps=8'],
'j8-plain':['-O2','-falign-jumps=8'],
'j8-cost0':['-O2','-frename-registers','-falign-jumps=8','-mbranch-cost=0'],
'j8-cost1':['-O2','-frename-registers','-falign-jumps=8','-mbranch-cost=1'],
'j8-cost2':['-O2','-frename-registers','-falign-jumps=8','-mbranch-cost=2'],
'j8-noguess':['-O2','-frename-registers','-falign-jumps=8','-fno-guess-branch-probability'],
'j8-nothread':['-O2','-frename-registers','-falign-jumps=8','-fno-thread-jumps'],
'j8-noifconv':['-O2','-frename-registers','-falign-jumps=8','-fno-if-conversion'],
'j8-noifconv2':['-O2','-frename-registers','-falign-jumps=8','-fno-if-conversion2'],
'j8-noreorder':['-O2','-frename-registers','-falign-jumps=8','-fno-reorder-blocks'],
'j8-nosched':['-O2','-frename-registers','-falign-jumps=8','-fno-schedule-insns2'],
'j8-noremove':['-O2','-frename-registers','-falign-jumps=8','-fno-regmove'],
'j8-label1':['-O2','-frename-registers','-falign-jumps=8','-falign-labels=1'],
'j8-loop1':['-O2','-frename-registers','-falign-jumps=8','-falign-loops=1'],
'j4':['-O2','-frename-registers','-falign-jumps=4'],
'j1':['-O2','-frename-registers','-falign-jumps=1'],
}
rows=[]
for name,replacement in forms.items():
 cpp=T/(name+'.cpp');cpp.write_text(base.replace(orig,replacement))
 for mode,flags in opts.items():
  obj=T/(name+'-'+mode+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
  if cp.returncode:
   print('COMPILER_ERROR',name,mode,cp.stderr[-90:],flush=True);continue
  nm=subprocess.check_output(['nm','-S',str(obj)],text=True)
  sym=next((s for s in nm.splitlines() if s.split()[-1:]==['_Z17IsAccumulatorablePN2IR13Operator_DataE']),'')
  size=int(sym.split()[1],16) if sym else -1
  if size!=77:rows.append((0,size,name,mode,'length'));continue
  res=m.run(obj,R/'vuc/config/ir_accumulatorable_probe.json')
  rows.append((res['exact_bytes'],size,name,mode,res['different_offsets'][:12]))
  if res['exact']:
   print('NEW_EXACT',cpp,mode,obj,flush=True);raise SystemExit(0)
 print('DONE',name,flush=True)
print('TOP')
for x in sorted(rows,reverse=True)[:45]:print(x,flush=True)
