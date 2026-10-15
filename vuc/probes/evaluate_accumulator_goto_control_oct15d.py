#!/usr/bin/env python3
"""VUC 77-byte accumulator source: explicit true/fallthrough code ownership.
Original last pair uses jne-next; jmp-success while reconstructed compiler
does je-success; jmp-next. Real C++03 goto/shared-tail source hypotheses,
never injected assembler, fixed register bindings or layout-only padding.
"""
from pathlib import Path
import importlib.util,itertools,subprocess,json,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/'vuc/config/ir_accumulatorable_probe.json').read_text())
base=(R/'vuc/probes/ir_accumulatorable_joint_oct14.cpp').read_text()
old=base[base.index('bool IsAccumulatorable('):]
intro='''bool IsAccumulatorable(IR::Operator_Data *op)
{
    if (__builtin_expect(op->kind == 11, 0)) {
        if (__builtin_expect(op->operand->kind == 3, 1)) return true;
    }
    if (__builtin_expect(op->kind == 13, 0)) {
        if (op->operand->kind == 3) return true;
    }
    if (op->kind == 12) {
        if (__builtin_expect(op->operand->kind == 3, 0)) return true;
    }
    return false;
}
'''
assert old==intro
variants={}
for final in (0,1,2):
 # true destination explicit named label shared by two/three cases
 for condname,cond in {
   'eq':'op->operand->kind == 3',
   'neq':'op->operand->kind != 3',
   'eq-likely':'__builtin_expect(op->operand->kind == 3, 1)',
   'eq-unlikely':'__builtin_expect(op->operand->kind == 3, 0)',
   'neq-likely':'__builtin_expect(op->operand->kind != 3, 1)',
   'neq-unlikely':'__builtin_expect(op->operand->kind != 3, 0)'
 }.items():
  call='goto done_true;' if final in (1,2) else 'return true;'
  subcall='goto done_true;' if final==2 else 'return true;'
  check=(f'if ({cond}) {call};' if 'neq' not in condname else f'if ({cond}) goto next_case11;\n        {call}')
  # for negative branch, goto next_case label in scope after first if.
  label=('next_case11:\n    ' if 'neq' in condname else '')
  tail=('\n done_true: return true;' if final else '')
  extra=old.replace('if (__builtin_expect(op->operand->kind == 3, 1)) return true;',check)
  if 'neq' in condname:
   extra=extra.replace('    if (__builtin_expect(op->kind == 13, 0))',f'    {label}if (__builtin_expect(op->kind == 13, 0))')
  extra=extra.replace('if (op->operand->kind == 3) return true;',f'if (op->operand->kind == 3) {subcall}')
  extra=extra.replace('if (__builtin_expect(op->operand->kind == 3, 0)) return true;',f'if (__builtin_expect(op->operand->kind == 3, 0)) {subcall}')
  if tail:extra=extra.replace('    return false;','    return false;'+tail)
  variants[f'{final}-{condname}']=base.replace(old,extra)
flags={
 'original':['-O2','-frename-registers','-falign-jumps=8'],
 'jump8l1':['-O2','-frename-registers','-falign-jumps=8','-falign-labels=1'],
 'jump8l4':['-O2','-frename-registers','-falign-jumps=8','-falign-labels=4'],
 'jump4':['-O2','-frename-registers','-falign-jumps=4'],
 'jump16':['-O2','-frename-registers','-falign-jumps=16'],
 'no-cross':['-O2','-frename-registers','-falign-jumps=8','-fno-crossjumping'],
 'no-reorder':['-O2','-frename-registers','-falign-jumps=8','-fno-reorder-blocks'],
}
T=Path('/tmp/vuc-binutils215/vuc-accumulator-goto-branches-oct15d');T.mkdir(parents=True,exist_ok=True)
raw,elf=m.decomp.load();sec=next(s for s in elf['sections'] if s['name']=='.text');addr=int(cfg['address'],0);ref=raw[sec['offset']+addr-sec['address']:sec['offset']+addr-sec['address']+77]
rows=[];sizes=collections.Counter();best=None
for (name,source),(fn,opts) in itertools.product(variants.items(),flags.items()):
 tag=f'{name}-{fn}';cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(source)
 p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:rows.append(dict(tag=tag,error=p.stderr[-135:]));continue
 try:bytes_,rel=m.prepare_named_function(obj.read_bytes(),cfg['function'],cfg['relocations'],addr)
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)));continue
 overlap=sum(x==y for x,y in zip(ref,bytes_));prefix=next((i for i,(x,y) in enumerate(zip(ref,bytes_)) if x!=y),min(len(ref),len(bytes_)))
 row=dict(tag=tag,length=len(bytes_),match=overlap,prefix=prefix,relocations=rel)
 rows.append(row);sizes[len(bytes_)]+=1
 if len(bytes_)==77 and rel==0 and (best is None or (overlap,prefix)>(best['match'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(source);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(opts)+'\n')
  if overlap==77:print('EXACT',json.dumps(row),flush=True);break
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best=best,top=sorted((r for r in rows if 'match' in r),key=lambda r:(r['match'],r['prefix']),reverse=True)[:12]),indent=2))
