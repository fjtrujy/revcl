#!/usr/bin/env python3
"""Near-exact Print(ASTFunCall*) GCC register allocation controls.

VUC original 98-byte printer differs from a semantically identical 98-byte
GCC 3.3.3 object by exactly 2 register-field bytes (EAX vs ECX). Test the
true Printer.cpp source unit, genuine inline callee reference lifetimes and
period compiler passes; retain original literal+relocation address checks.
No dummy predecessor, artificial asm or synthetic register pinning.
"""
from pathlib import Path
import itertools,importlib.util,subprocess,json,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=R/'vuc/config/print_ast_fun_context_oct14_probe.json'
src=(R/'vuc/src/Printer.cpp').read_text().replace(
 '"../src/ASTNodeList.h"','"'+str(R/'vuc/src/ASTNodeList.h')+'"')
start='    Print(call->callee, output);'
assert start in src
ops={
 'direct':start,
 'callee-local':'    ASTLValExpr *callee = call->callee;\n    Print(callee, output);',
 'callee-const':'    ASTLValExpr *const callee = call->callee;\n    Print(callee, output);',
 'callee-reference':'    ASTLValExpr *&callee = call->callee;\n    Print(callee, output);',
 'callee-constref':'    ASTLValExpr *const &callee = call->callee;\n    Print(callee, output);',
 'callee-pointer':'    ASTLValExpr **callee = &call->callee;\n    Print(*callee, output);',
 'callee-pointerconst':'    ASTLValExpr *const *callee = &call->callee;\n    Print(*callee, output);',
 'callee-accessor':'    Print(call->callee, output);',
}
flags={
 'o2':['-O2'],
 'rename':['-O2','-frename-registers'],
 'no-regmove':['-O2','-fno-regmove'],
 'no-cse-follow':['-O2','-fno-cse-follow-jumps'],
 'no-gcse':['-O2','-fno-gcse'],
 'no-ifconvert':['-O2','-fno-if-conversion'],
 'no-sched2':['-O2','-fno-schedule-insns2'],
 'no-peephole2':['-O2','-fno-peephole2'],
 'no-reorder':['-O2','-fno-reorder-blocks'],
 'no-crossjump':['-O2','-fno-crossjumping'],
 'no-guess':['-O2','-fno-guess-branch-probability'],
 'align1':['-O2','-falign-jumps=1'],
}
T=Path('/tmp/vuc-binutils215/printer-register-source-owner-next-oct14')
T.mkdir(parents=True,exist_ok=True)
scores=[];best=None;lengths=collections.Counter()
for (variant,op),(label,opts),snapshot in itertools.product(
 ops.items(),flags.items(),('jan25','dec06')):
 key=f'{variant}-{label}-{snapshot}'
 cpp=T/(key+'.cpp');obj=T/(key+'.o')
 candidate=src.replace(start,op)
 if variant=='callee-accessor':
  needle='    ASTNodeList<ASTExpr *> arguments;'
  candidate=candidate.replace(needle,needle+'\n    ASTLValExpr *calleeValue() const { return callee; }')
  candidate=candidate.replace('Print(call->callee, output);', 'Print(call->calleeValue(), output);')
 cpp.write_text(candidate)
 cp=subprocess.run(['/tmp/vuc-binutils215/g++-'+snapshot+'-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if cp.returncode:
  scores.append(dict(key=key,error=cp.stderr[-150:]));continue
 try:res=m.run(obj,cfg)
 except ValueError as e:
  scores.append(dict(key=key,error=str(e)));continue
 lengths[res['length']]+=1
 row=dict(key=key,length=res['length'],matched=res['exact_bytes'],exact=res['exact'],different_offsets=res['different_offsets'][:10])
 scores.append(row)
 if res['length']==98 and (best is None or res['exact_bytes']>best['matched']):
  best=row
  (T/'best.cpp').write_text(candidate);(T/'best.o').write_bytes(obj.read_bytes())
(T/'results.json').write_text(json.dumps(scores,indent=2)+'\n')
print(json.dumps(dict(attempted=len(scores),lengths=dict(lengths),best_original_length=best,top=sorted((r for r in scores if 'matched' in r),key=lambda r:r['matched'],reverse=True)[:12]),indent=2))
