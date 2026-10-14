#!/usr/bin/env python3
"""Original-ELF VUC AND vector per-operand branch-likelihood diagnostics.

GCC 3.3.3 code placement is especially sensitive to branch frequency.
Test legal C++ expression-level predictions at each real operand, without
padding, register binding, binary patching or invented predecessor calls.
A winning byte count is valid only at original length 325 + true relocation.
"""
from pathlib import Path
import itertools,importlib.util,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/'vuc/config/value_logical_and_partial_oct14_probe.json').read_text())
raw,elf=m.decomp.load();sec=next(s for s in elf['sections'] if s['name']=='.text')
addr=int(cfg['address'],0);orig=raw[sec['offset']+addr-sec['address']:sec['offset']+addr-sec['address']+325]
source=(R/cfg['source']).read_text();start=source.index('   case 4:');end=source.index(';break;',start)
fore,tail=source[:start],source[end:]
vector=source[start:end]
replacements={}
for i in range(4):
 for owner in ('result','temporary'):
  term=f'{owner}.values[{i}] != 0.0f'
  assert term in vector
  replacements[f'{owner[:1]}{i}']=term
flags={
 'chosen':['-O2','-frename-registers','-ffast-math','-falign-jumps=16','-falign-labels=1','-falign-loops=1'],
 'j8-l1':['-O2','-frename-registers','-ffast-math','-falign-jumps=8','-falign-labels=1','-falign-loops=1'],
 'j1-l8':['-O2','-frename-registers','-ffast-math','-falign-jumps=1','-falign-labels=8','-falign-loops=1'],
 'j1-l16':['-O2','-frename-registers','-ffast-math','-falign-jumps=1','-falign-labels=16','-falign-loops=1']
}
T=Path('/tmp/vuc-binutils215/value-and-operand-prediction-oct14');T.mkdir(parents=True,exist_ok=True)
scores=[];sizes=collections.Counter();best=None
for key,term in replacements.items():
 for sign in (0,1):
  replacement='__builtin_expect(('+term+'),'+str(sign)+')'
  candidate=fore+vector.replace(term,replacement,1)+tail
  for opt,opts in flags.items():
   tag=f'{key}-{sign}-{opt}';cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(candidate)
   cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
   if cp.returncode:scores.append(dict(tag=tag,error=cp.stderr[-160:]));continue
   try:body,rel=m.prepare_named_function(obj.read_bytes(),cfg['function'],cfg['relocations'],addr)
   except ValueError as e:scores.append(dict(tag=tag,error=str(e)));continue
   overlap=sum(a==b for a,b in zip(body,orig))
   prefix=next((i for i,(a,b) in enumerate(zip(body,orig)) if a!=b),min(len(body),len(orig)))
   row=dict(tag=tag,length=len(body),relocations=rel,overlap=overlap,prefix=prefix)
   scores.append(row);sizes[len(body)]+=1
   if len(body)==325 and rel==1 and (best is None or (overlap,prefix)>(best['overlap'],best['prefix'])):
    best=row;(T/'best.cpp').write_text(candidate);(T/'best.flags').write_text(' '.join(opts)+'\n');(T/'best.o').write_bytes(obj.read_bytes())
(T/'results.json').write_text(json.dumps(scores,indent=2)+'\n')
print(json.dumps(dict(attempted=len(scores),lengths=dict(sizes),best_original_length=best,top=sorted((s for s in scores if 'overlap' in s),key=lambda r:(r['overlap'],r['prefix']),reverse=True)[:12]),indent=2))
