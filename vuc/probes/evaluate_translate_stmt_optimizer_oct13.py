#!/usr/bin/env python3
"""Historical optimizer controls for genuine Translate AST statement-list source.

VCL main identified a narrow -fno-gcse owner fix; independently test
whether VUC's GCC 3.3.3 emits the original 119-byte loop and IR::Seq_Data.
Acceptance requires original size and all three actual relocations.
"""
from pathlib import Path
import importlib.util,subprocess
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/translate-stmt-optimizer-oct13');T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
p=R/'vuc/probes/translate_stmt_list.cpp'
cfg=R/'vuc/config/translate_stmt_list_probe.json'
variants={
 'baseline':[],
 'no-gcse':['-fno-gcse'],
 'no-cse-follow':['-fno-cse-follow-jumps'],
 'no-ifconv':['-fno-if-conversion'],
 'no-ifconv2':['-fno-if-conversion2'],
 'no-regmove':['-fno-regmove'],
 'no-schedule2':['-fno-schedule-insns2'],
 'no-peephole2':['-fno-peephole2'],
 'no-reorder':['-fno-reorder-blocks'],
 'no-expensive':['-fno-expensive-optimizations'],
 'no-guess':['-fno-guess-branch-probability'],
 'no-inline':['-fno-inline'],
 'no-builtin':['-fno-builtin'],
 'no-strict-alias':['-fno-strict-aliasing'],
 'no-crossjump':['-fno-crossjumping'],
 'no-caller-saves':['-fno-caller-saves'],
 'no-defer-pop':['-fno-defer-pop'],
 'no-strength-reduce':['-fno-strength-reduce'],
 'no-common':['-fno-common'],
 'no-optimize-sibling':['-fno-optimize-sibling-calls'],
 'merge':['-fmerge-all-constants'],
}
scores=[]
for name,extra in variants.items():
 for date,c in [('jan25','/tmp/vuc-binutils215/g++-jan25-as215'),('dec06','/tmp/vuc-binutils215/g++-dec06-as215')]:
  for ren in (True,False):
   opt=['-O2']+(['-frename-registers'] if ren else [])+extra
   id=name+'-'+date+('-ren' if ren else '-plain')
   obj=T/(id+'.o')
   pr=subprocess.run([c,*opt,'-c',str(p),'-o',str(obj)],capture_output=True,text=True)
   if pr.returncode:print('COMPILE_FAIL',id,pr.stderr[-150:],flush=True);continue
   sym=next((line for line in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines() if line.endswith('_Z9TranslateR11ASTNodeListIP7ASTStmtE')),'')
   size=int(sym.split()[1],16) if sym else -1
   if size==119:
    try:
     result=m.run(obj,cfg)
     scores.append((result['exact_bytes'],size,id))
     print('ORIGINAL_LENGTH',id,result['exact_bytes'],result['different_offsets'][:12],flush=True)
     if result['exact']:print('NEW_EXACT',id,flush=True);raise SystemExit(0)
    except ValueError as e:print('REJECT',id,str(e)[:110],flush=True)
   else:scores.append((0,size,id))
 print('DONE',name,flush=True)
print('BEST')
for row in sorted(scores,reverse=True)[:30]:print(row)
