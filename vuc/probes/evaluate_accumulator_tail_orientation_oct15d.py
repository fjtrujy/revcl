#!/usr/bin/env python3
"""VUC IsAccumulatorable final 3-byte CFG orientation, C++03 source hypotheses.
Original 77-byte instruction code differs only by last conditional branch
orientation/displacement. Probe meaningful positive vs negative tests and
equivalent nested/goto flows. No explicit assembler / register forcing.
"""
from pathlib import Path
import itertools,subprocess,json,importlib.util,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
conf=json.loads((R/'vuc/config/ir_accumulatorable_probe.json').read_text())
src=(R/conf['source']).read_text()
anchor='''    if (__builtin_expect(op->kind == 11, 0)) {
        if (__builtin_expect(op->operand->kind == 3, 1)) return true;
    }'''
assert anchor in src
inner={
 'positive-likely': 'if (__builtin_expect(op->operand->kind == 3, 1)) return true;',
 'positive-unlikely': 'if (__builtin_expect(op->operand->kind == 3, 0)) return true;',
 'positive-plain': 'if (op->operand->kind == 3) return true;',
 'negative-unlikely-else': 'if (__builtin_expect(op->operand->kind != 3, 0)) {} else return true;',
 'negative-likely-else': 'if (__builtin_expect(op->operand->kind != 3, 1)) {} else return true;',
 'negative-plain-else': 'if (op->operand->kind != 3) {} else return true;',
 'negative-early-false': 'if (__builtin_expect(op->operand->kind != 3, 0)) {} else { return true; }',
 'negative-early-true': 'if (__builtin_expect(op->operand->kind != 3, 1)) {} else { return true; }',
 'negative-fallthrough': 'if (__builtin_expect(!(op->operand->kind == 3), 0)) { } else return true;',
 'negative-fallthrough1': 'if (__builtin_expect(!(op->operand->kind == 3), 1)) { } else return true;',
 'negated-boolean': 'if (!__builtin_expect(op->operand->kind != 3, 0)) return true;',
 'negated-boolean1': 'if (!__builtin_expect(op->operand->kind != 3, 1)) return true;',
 'logical-bool': 'if (__builtin_expect(!!(op->operand->kind == 3), 1)) return true;',
 'ternary': 'if (__builtin_expect(op->operand->kind == 3, 1) ? true : false) return true;',
 'ternary2': 'if (__builtin_expect(op->operand->kind != 3, 0) ? false : true) return true;',
 'local-bool': 'bool result = op->operand->kind == 3; if (__builtin_expect(result, 1)) return true;',
 'local-bool-neg': 'bool result = op->operand->kind != 3; if (!__builtin_expect(result, 0)) return true;',
 'local-int': 'int value = op->operand->kind; if (__builtin_expect(value == 3, 1)) return true;',
 'local-int-negative': 'int value = op->operand->kind; if (__builtin_expect(value != 3, 0)) {} else return true;',
 'return-int-cast': 'if (__builtin_expect((int)(op->operand->kind == 3), 1)) return true;',
 'bit-operand': 'if (__builtin_expect((unsigned)op->operand->kind == 3U, 1)) return true;',
 'cmp-not': 'if (__builtin_expect((op->operand->kind < 3) || (op->operand->kind > 3), 0)) {} else return true;',
}
outer={f'kind-{x}':f'__builtin_expect(op->kind == 11, {x})' for x in (0,1)}
outer['kind-plain']='op->kind == 11'
flags={
 'original':['-O2','-frename-registers','-falign-jumps=8'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j8l1':['-O2','-frename-registers','-falign-jumps=8','-falign-labels=1'],
 'j8l2':['-O2','-frename-registers','-falign-jumps=8','-falign-labels=2'],
 'j8no-cross':['-O2','-frename-registers','-falign-jumps=8','-fno-crossjumping'],
 'j8no-reorder':['-O2','-frename-registers','-falign-jumps=8','-fno-reorder-blocks'],
 'j8nosched':['-O2','-frename-registers','-falign-jumps=8','-fno-schedule-insns2'],
 'j8no-guess':['-O2','-frename-registers','-falign-jumps=8','-fno-guess-branch-probability'],
}
T=Path('/tmp/vuc-binutils215/vuc-accumulator-last-branch-oct15d');T.mkdir(exist_ok=True,parents=True)
raw,elf=m.decomp.load();sect=next(s for s in elf['sections'] if s['name']=='.text');addr=int(conf['address'],0);ref=raw[sect['offset']+addr-sect['address']:sect['offset']+addr-sect['address']+conf['size']]
scores=[];sizes=collections.Counter();best=None
for (inner_name,inner_src),(outer_name,outer_src),(flagname,opt) in itertools.product(inner.items(),outer.items(),flags.items()):
 tag=f'{inner_name}-{outer_name}-{flagname}'
 body='    if ('+outer_src+') {\n        '+inner_src+'\n    }'
 candidate=src.replace(anchor,body)
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(candidate)
 p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opt,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:scores.append(dict(tag=tag,error=p.stderr[-120:]));continue
 try:b,r=m.prepare_named_function(obj.read_bytes(),conf['function'],conf['relocations'],addr)
 except ValueError as e:scores.append(dict(tag=tag,error=str(e)));continue
 match=sum(x==y for x,y in zip(ref,b));prefix=next((i for i,(x,y) in enumerate(zip(ref,b)) if x!=y),min(len(ref),len(b)))
 row=dict(tag=tag,length=len(b),match=match,prefix=prefix,relocations=r)
 scores.append(row);sizes[len(b)]+=1
 if len(b)==conf['size'] and r==0 and (best is None or (match,prefix)>(best['match'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(candidate);(T/'best.flags').write_text(' '.join(opt)+'\n');(T/'best.o').write_bytes(obj.read_bytes())
  if match==conf['size']:print('EXACT',json.dumps(row),flush=True);break
(T/'results.json').write_text(json.dumps(scores,indent=2)+'\n')
print(json.dumps(dict(attempted=len(scores),sizes=dict(sizes),best=best,top=sorted((r for r in scores if 'match' in r),key=lambda r:(r['match'],r['prefix']),reverse=True)[:15]),indent=2))
