#!/usr/bin/env python3
"""Historical GCC 3.3.3 declaration-level EH hypotheses for VUC Scope copy.
Scratch-only copies of provisional headers; no accepted source is modified.
"""
from pathlib import Path
import importlib.util,itertools,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('match',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/'vuc/config/scope_copy_prefix_diagnostic_oct14_probe.json').read_text())
raw,target=m.decomp.load()
addr=int(cfg['address'],0)
sec=next(s for s in target['sections'] if s['name']=='.text')
orig=raw[sec['offset']+addr-sec['address']:sec['offset']+addr-sec['address']+120]
source=(R/cfg['source']).read_text()
header=(R/'vuc/src/MLHashTable.h').read_text()
hash_h=str(R/'vuc/src/MLHashFunGameString.h')
scratch=Path('/tmp/vuc-binutils215/scope-exception-declarations-oct14')
scratch.mkdir(parents=True,exist_ok=True)
decls={'none':'','throw':' throw()','nothrow':' __attribute__((nothrow))'}
flags={'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
 'j8-nr':['-O2','-frename-registers','-falign-jumps=8','-fno-reorder-blocks'],
 'j4-no-cse':['-O2','-frename-registers','-falign-jumps=4','-fno-cse-follow-jumps']}
rows=[];lengths=collections.Counter();best=None
for (copy_name,copy),(clear_name,clear),(dtor_name,dtor),(flagname,opts) in itertools.product(
 decls.items(),decls.items(),decls.items(),flags.items()):
 name=f'copy-{copy_name}-clear-{clear_name}-dtor-{dtor_name}-{flagname}'
 h=header.replace('MLHashTable(const MLHashTable &other);',f'MLHashTable(const MLHashTable &other){copy};')
 h=h.replace('    void clear();',f'    void clear(){clear};')
 h=h.replace('~MLHashTable() { clear(); }',f'~MLHashTable(){dtor} {{ clear(); }}')
 include=scratch/(name+'.h');include.write_text(h)
 src=source.replace('#include "../src/MLHashFunGameString.h"','#include "'+hash_h+'"')
 src=src.replace('#include "../src/MLHashTable.h"','#include "'+str(include)+'"')
 cpp=scratch/(name+'.cpp');cpp.write_text(src)
 obj=scratch/(name+'.o')
 cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if cp.returncode:
  rows.append(dict(name=name,error=cp.stderr[:130]));continue
 try:code,rel=m.prepare_named_function(obj.read_bytes(),cfg['function'],cfg['relocations'],addr)
 except Exception as e:
  rows.append(dict(name=name,error=str(e)));continue
 lengths[len(code)]+=1
 prefix=next((i for i,(a,b) in enumerate(zip(code,orig)) if a!=b),min(len(code),len(orig)))
 overlap=sum(a==b for a,b in zip(code,orig))
 row=dict(name=name,size=len(code),relocations=rel,prefix=prefix,overlap=overlap)
 rows.append(row)
 if len(code)==120 and rel==6 and (best is None or (overlap,prefix)>(best['overlap'],best['prefix'])):
  best=row
  (scratch/'best.cpp').write_text(cpp.read_text())
  (scratch/'best.h').write_text(h)
(scratch/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(lengths),best_original_length=best,best_prefix=sorted((r for r in rows if 'overlap'in r),key=lambda x:(x['prefix'],x['overlap']),reverse=True)[:8],result_path=str(scratch/'results.json')),indent=2))
