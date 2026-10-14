#!/usr/bin/env python3
"""Test factual no-throw MLGameString copy vs GCC EH in 513-bucket constructor.

The recovered MLGameString copy ctor (0x08071770) consists of one pointer
assignment, so exception-spec hypotheses are justified by real behavior.
Work on scratch headers; do not alter the shared accepted VUC declaration.
"""
from pathlib import Path
import importlib.util,itertools,subprocess,json,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('match',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
config=json.loads((R/'vuc/config/hash_copy_constructor_probe.json').read_text())
raw,elf=m.decomp.load();sec=next(s for s in elf['sections'] if s['name']=='.text')
addr=int(config['address'],0);ref=raw[sec['offset']+addr-sec['address']:sec['offset']+addr-sec['address']+166]
s=Path('/tmp/vuc-binutils215/hashcopy-real-nothrow-oct14');s.mkdir(exist_ok=True,parents=True)
string_h=(R/'vuc/src/MLGameString.h').read_text()
hash_h=(R/'vuc/src/MLHashTable.h').read_text()
hashfun_h=(R/'vuc/src/MLHashFunGameString.h').read_text()
old=(R/'vuc/probes/hash_copy_constructor.cpp').read_text()
for f in ('MLHashTable.h','MLHashFunGameString.h'):
 old=old.replace('#include "../src/'+f+'"','#include "'+f+'"')
string_specs={'none':'','throw':' throw()','attribute':' __attribute__((nothrow))'}
hash_specs={'none':'','throw':' throw()'}
flags={
 'base':['-O2','-frename-registers'],
 'j1':['-O2','-frename-registers','-falign-jumps=1'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
 'no-reorder':['-O2','-frename-registers','-fno-reorder-blocks'],
 'plain':['-O2'],
}
results=[];counts=collections.Counter();best=None
for (sn,spec),(hn,hspec),(fn,fs) in itertools.product(string_specs.items(),hash_specs.items(),flags.items()):
 name=f'{sn}-{hn}-{fn}'
 folder=s/(sn+'-'+hn);folder.mkdir(exist_ok=True)
 (folder/'MLGameString.h').write_text(string_h.replace('MLGameString(const MLGameString &other);','MLGameString(const MLGameString &other)'+spec+';'))
 (folder/'MLHashTable.h').write_text(hash_h.replace('MLHashTable(const MLHashTable &other);','MLHashTable(const MLHashTable &other)'+hspec+';'))
 (folder/'MLHashFunGameString.h').write_text(hashfun_h)
 src=folder/'candidate.cpp';src.write_text(old)
 o=s/(name+'.o')
 comp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*fs,'-c',str(src),'-o',str(o)],capture_output=True,text=True)
 if comp.returncode:
  results.append(dict(name=name,error=comp.stderr[-120:]));continue
 try:
  code,rels=m.prepare_named_function(o.read_bytes(),config['function'],config['relocations'],addr)
 except ValueError as e:
  results.append(dict(name=name,error=str(e)));continue
 overlap=sum(a==b for a,b in zip(code,ref));counts[len(code)]+=1
 prefix=next((i for i,(a,b) in enumerate(zip(code,ref)) if a!=b),min(len(code),len(ref)))
 row=dict(name=name,length=len(code),relocations=rels,overlap=overlap,prefix=prefix)
 results.append(row)
 if len(code)==166 and rels==4 and (best is None or (overlap,prefix)>(best['overlap'],best['prefix'])):
  best=row
  (s/'best.cpp').write_text(src.read_text());(s/'best.o').write_bytes(o.read_bytes())
(s/'results.json').write_text(json.dumps(results,indent=2)+'\n')
print(json.dumps(dict(attempted=len(results),sizes=dict(counts),best_original_length=best,top=sorted((r for r in results if 'overlap' in r),key=lambda r:(r['overlap'],r['prefix']),reverse=True)[:10],path=str(s/'results.json')),indent=2))
