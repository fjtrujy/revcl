#!/usr/bin/env python3
"""Original x86 166-byte VUC hash-table copy constructor: Node argument order.

The recovered Node layout has key, hash, value and next fields. Test period-
plausible C++03 explicit constructor parameter ordering and source pointer
lifetimes without affecting the actual header or crafting instructions.
Only authenticate 166-byte original-size functions and all four relocations.
"""
from pathlib import Path
import itertools,importlib.util,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/'vuc/config/hash_copy_node_args_partial_oct14_probe.json').read_text())
raw,elf=m.decomp.load();s=next(x for x in elf['sections'] if x['name']=='.text')
addr=int(cfg['address'],0);orig=raw[s['offset']+addr-s['address']:s['offset']+addr-s['address']+166]
header=(R/'vuc/probes/MLHashTableCopyNodePartial.h').read_text()
source=(R/'vuc/probes/hash_copy_node_args_partial_oct14.cpp').read_text()
target='Node(const Key &k, unsigned h, Value v, Node *n) : key(k), hash(h), value(v), next(n) {}'
assert target in header
source_call='new Node(node->key, node->hash, node->value, node->next)'
assert source_call in source
definitions={
 'key':('const Key &k','node->key'),
 'hash':('unsigned h','node->hash'),
 'value':('Value v','node->value'),
 'next':('Node *n','node->next')
}
flags={
 'j1':['-O2','-frename-registers','-falign-jumps=1'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j8':['-O2','-frename-registers','-falign-jumps=8']
}
T=Path('/tmp/vuc-binutils215/hash-copy-node-arg-order-oct14');T.mkdir(parents=True,exist_ok=True)
scores=[];lengths=collections.Counter();best=None
for order,fl in itertools.product(itertools.permutations(definitions),flags):
 tag='-'.join(order)+'-'+fl
 signature='Node('+', '.join(definitions[x][0] for x in order)+') : key(k), hash(h), value(v), next(n) {}'
 body=header.replace(target,signature)
 args='new Node('+', '.join(definitions[x][1] for x in order)+')'
 new_source=source.replace(source_call,args)
 h=T/(tag+'.h');cpp=T/(tag+'.cpp');o=T/(tag+'.o')
 h.write_text(body);cpp.write_text(new_source.replace('#include "MLHashTableCopyNodePartial.h"','#include "'+str(h)+'"').replace('#include "../src/MLHashFunGameString.h"','#include "'+str(R/'vuc/src/MLHashFunGameString.h')+'"'))
 run=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags[fl],'-c',str(cpp),'-o',str(o)],capture_output=True,text=True)
 if run.returncode:scores.append(dict(tag=tag,error=run.stderr[-140:]));continue
 try:data,rel=m.prepare_named_function(o.read_bytes(),cfg['function'],cfg['relocations'],addr)
 except ValueError as ex:scores.append(dict(tag=tag,error=str(ex)));continue
 count=sum(a==b for a,b in zip(data,orig))
 prefix=next((i for i,(a,b) in enumerate(zip(data,orig)) if a!=b),min(len(data),len(orig)))
 row=dict(tag=tag,length=len(data),relocations=rel,overlap=count,prefix=prefix)
 scores.append(row);lengths[len(data)]+=1
 if len(data)==166 and rel==4 and (best is None or (count,prefix)>(best['overlap'],best['prefix'])):
  best=row;(T/'best.h').write_text(body);(T/'best.cpp').write_text(new_source);(T/'best.flags').write_text(' '.join(flags[fl])+'\n');(T/'best.o').write_bytes(o.read_bytes())
(T/'results.json').write_text(json.dumps(scores,indent=2)+'\n')
print(json.dumps(dict(attempted=len(scores),lengths=dict(lengths),best_original_length=best,top=sorted((x for x in scores if 'overlap' in x),key=lambda r:(r['overlap'],r['prefix']),reverse=True)[:10]),indent=2))
