#!/usr/bin/env python3
"""VUC accumulator 74/77: authentic-looking inline helper/branch topology.

Test pure source-level predicates as GCC 3.3.3 may propagate source
branch prediction differently across inline function boundaries.
No forcing branch assembly, inserted padding, or fabricated relocations.
"""
from pathlib import Path
import subprocess,itertools,importlib.util,collections
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/accum-inline-predicates-oct14');T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
header=(R/'vuc/probes/ir_accumulatorable_joint_oct14.cpp').read_text()
header=header[:header.index('bool IsAccumulatorable(')]
function=(R/'vuc/probes/ir_accumulatorable_joint_oct14.cpp').read_text().split('bool IsAccumulatorable(',1)[1]
function='bool IsAccumulatorable('+function
old='op->operand->kind == 3'
helper_variants={
  'static_inline':'static inline bool HasAccumulatorType(IR::Operator_Data *p) { return p->operand->kind == 3; }',
  'always_inline':'static inline __attribute__((always_inline)) bool HasAccumulatorType(IR::Operator_Data *p) { return p->operand->kind == 3; }',
  'static_inline_not':'static inline bool HasAccumulatorType(IR::Operator_Data *p) { return !(p->operand->kind != 3); }',
  'static_inline_bool':'static inline bool HasAccumulatorType(IR::Operator_Data *p) { bool result=(p->operand->kind == 3); return result; }',
  'static_inline_expr':'static inline bool HasAccumulatorType(IR::Operator_Data *p) { return __builtin_expect(p->operand->kind == 3,1); }',
  'static_inline_expr0':'static inline bool HasAccumulatorType(IR::Operator_Data *p) { return __builtin_expect(p->operand->kind == 3,0); }',
  'static_inline_notexpr':'static inline bool HasAccumulatorType(IR::Operator_Data *p) { return !__builtin_expect(p->operand->kind != 3,0); }',
  'static_inline_ref':'static inline bool HasAccumulatorType(IR::Operator_Data const *p) { return p->operand->kind == 3; }',
  'template_inline':'template<class T> inline bool HasAccumulatorType(T *p) {return p->operand->kind == 3;}',
}
forms={
  'h11-outer-pred':'if (__builtin_expect(HasAccumulatorType(op),1))',
  'h11-outer-pred0':'if (__builtin_expect(HasAccumulatorType(op),0))',
  'h11-plain':'if (HasAccumulatorType(op))',
  'h11-ref':'if (__builtin_expect(HasAccumulatorType((IR::Operator_Data*)op),1))',
  'h11-bool':'bool cond=HasAccumulatorType(op); if (__builtin_expect(cond,1))',
  'h11-int':'int cond=HasAccumulatorType(op); if (__builtin_expect(cond,1))',
  'h11-not':'if (!__builtin_expect(!HasAccumulatorType(op),0))',
  'h11-bool-and':'if (__builtin_expect(HasAccumulatorType(op) && true,1))',
}
bodyold='if (__builtin_expect('+old+', 1))'
assert bodyold in function
opts={
  'j8':['-O2','-frename-registers','-falign-jumps=8'],
  'j8-plain':['-O2','-falign-jumps=8'],
  'j8-no-sched':['-O2','-frename-registers','-falign-jumps=8','-fno-schedule-insns2']
}
records=[];sizes=collections.Counter()
for (name,helper),(form,replacement) in itertools.product(helper_variants.items(),forms.items()):
  if form=='h11-bool-and':
    # GCC rejects an explicit saved bool in an if-body expression; this
    # is still a source-level inline predicate.
    pass
  src=header+'\n'+helper+'\n'+function.replace(bodyold,replacement,1)
  p=T/(name+'-'+form+'.cpp');p.write_text(src)
  for flag,flags in opts.items():
    obj=T/(name+'-'+form+'-'+flag+'.o')
    cc=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(p),'-o',str(obj)],capture_output=True,text=True)
    if cc.returncode:
      print('COMPILE_ERROR',name,form,flag,cc.stderr[-100:],flush=True);continue
    symbol=next((s for s in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines()
                 if s.split()[-1:]==['_Z17IsAccumulatorablePN2IR13Operator_DataE']),'')
    size=int(symbol.split()[1],16) if symbol else -1;sizes[size]+=1
    if size!=77:continue
    try:
      r=m.run(obj,R/'vuc/config/ir_accumulatorable_probe.json')
      records.append((r['exact_bytes'],name,form,flag,r['different_offsets'][:12]))
      if r['exact']:
        print('NEW_EXACT',p,flag,obj,flush=True);raise SystemExit(0)
    except ValueError as e:
      records.append((0,name,form,flag,str(e)[:110]))
  print('DONE',name,form,flush=True)
print('LENGTHS',sizes.most_common(18))
print('TOP')
for row in sorted(records,reverse=True)[:36]:print(row,flush=True)
