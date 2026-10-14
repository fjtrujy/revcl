#!/usr/bin/env python3
"""Real C++03 boolean-lane grouping/short-circuit ownership for VUC Value &&.

Original ELF tests the eight float inputs in interleaved pair order.
This checks plausible source constructs with no byte patching, padding,
register-binding asm or artificial predecessor functions.
"""
from pathlib import Path
import importlib.util,itertools,subprocess,json,collections
R=Path(__file__).resolve().parents[2];sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/'vuc/config/value_logical_and_partial_oct14_probe.json').read_text())
raw,elf=m.decomp.load();sec=next(x for x in elf['sections'] if x['name']=='.text')
addr=int(cfg['address'],0);original=raw[sec['offset']+addr-sec['address']:sec['offset']+addr-sec['address']+325]
base=(R/cfg['source']).read_text()
start=base.index('   case 4:');end=base.index(';break;',start)+len(';break;')
terms=[f'(result.values[{i}] != 0.0f && temporary.values[{i}] != 0.0f)' for i in range(4)]
members=[f'(result.values[{i}] != 0.0f)' if pos==0 else f'(temporary.values[{i}] != 0.0f)' for i in range(4) for pos in range(2)]
expressions={
 'pair-left':' && '.join(f'({t})' for t in terms),
 'pair-right': '('+' && '.join(f'({t})' for t in terms[:2])+') && ('+' && '.join(f'({t})' for t in terms[2:])+')',
 'all-eight':' && '.join(members),
 'ternary': '('+' && '.join(terms)+') ? 1 : 0',
 'bool-cast':'bool('+' && '.join(terms)+')',
 'local-pairs':None,
 'early-false':None,
 'integer-local':None,
}
def form(name,expr):
 if name=='local-pairs':
  return '   case 4: {\n'+''.join(f'    bool lane{i} = {terms[i]};\n' for i in range(4))+'    result.bits = lane0 && lane1 && lane2 && lane3; break;\n   }'
 if name=='early-false':
  return '   case 4: {\n    result.bits=0;\n    if ('+' && '.join(terms)+') result.bits=1;\n    break;\n   }'
 if name=='integer-local':
  return '   case 4: {\n    int truth = ('+' && '.join(terms)+');\n    result.bits=truth;\n    break;\n   }'
 return '   case 4: result.bits='+expr+';break;'
options={
 'chosen':['-O2','-frename-registers','-ffast-math','-falign-jumps=16','-falign-labels=1','-falign-loops=1'],
 'old':['-O2','-frename-registers','-ffast-math','-falign-jumps=1','-falign-labels=8','-falign-loops=1'],
 'or-flags':['-O2','-frename-registers','-ffast-math','-falign-jumps=8','-falign-labels=1','-falign-loops=4'],
 'l16':['-O2','-frename-registers','-ffast-math','-falign-jumps=1','-falign-labels=16','-falign-loops=1'],
}
T=Path('/tmp/vuc-binutils215/value-and-boolean-group-source');T.mkdir(parents=True,exist_ok=True)
recs=[];counts=collections.Counter();best=None
for (name,expr),(flag,opts),snap in itertools.product(expressions.items(),options.items(),('jan25','dec06')):
 tag=f'{name}-{flag}-{snap}'
 source=base[:start]+form(name,expr)+base[end:]
 cpp=T/(tag+'.cpp');o=T/(tag+'.o');cpp.write_text(source)
 cp=subprocess.run(['/tmp/vuc-binutils215/g++-'+snap+'-as215',*opts,'-c',str(cpp),'-o',str(o)],capture_output=True,text=True)
 if cp.returncode:recs.append(dict(tag=tag,error=cp.stderr[-150:]));continue
 try:body,rels=m.prepare_named_function(o.read_bytes(),cfg['function'],cfg['relocations'],addr)
 except ValueError as e:recs.append(dict(tag=tag,error=str(e)));continue
 score=sum(a==b for a,b in zip(body,original))
 prefix=next((i for i,(a,b) in enumerate(zip(body,original)) if a!=b),min(len(body),len(original)))
 r=dict(tag=tag,length=len(body),relocations=rels,overlap=score,prefix=prefix)
 recs.append(r);counts[len(body)]+=1
 if len(body)==325 and rels==1 and (best is None or (score,prefix)>(best['overlap'],best['prefix'])):
  best=r;(T/'best.cpp').write_text(source);(T/'best.o').write_bytes(o.read_bytes());(T/'best.flags').write_text(' '.join(opts)+'\n')
(T/'results.json').write_text(json.dumps(recs,indent=2)+'\n')
print(json.dumps(dict(attempted=len(recs),sizes=dict(counts),best=best,top=sorted((x for x in recs if 'overlap' in x),key=lambda x:(x['overlap'],x['prefix']),reverse=True)[:10]),indent=2))
