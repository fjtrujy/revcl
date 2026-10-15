#!/usr/bin/env python3
"""VUC Translate(ASTNodeList<ASTStmt*>&) GCC3.3 branch alignment & crossjump probes.
Original 119-byte function: authentic Translate(ASTStmt*), operator new
and Seq_Data vtable references required. All hypotheses genuine C++03
branch probability/optimization controls, no fake source padding.
"""
from pathlib import Path
import itertools,importlib.util,subprocess,json,collections
R=Path(__file__).resolve().parents[2];sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
conf=json.loads((R/'vuc/config/translate_stmt_list_partial_oct14_probe.json').read_text())
raw,elf=m.decomp.load();s=next(x for x in elf['sections'] if x['name']=='.text');a=int(conf['address'],0);ref=raw[s['offset']+a-s['address']:s['offset']+a-s['address']+119]
base=(R/'vuc/probes/translate_stmt_list_partial_oct14.cpp').read_text().replace('#include "../src/ASTNodeList.h"','#include "'+str(R/'vuc/src/ASTNodeList.h')+'"')
old=['__builtin_expect(i<statements.length, 1)','__builtin_expect(item != 0, 0)','__builtin_expect(result != 0, 0)']
flags={
 'base':['-O2'],
 'rename':['-O2','-frename-registers'],
 'j1':['-O2','-falign-jumps=1'],
 'j4':['-O2','-falign-jumps=4'],
 'j8':['-O2','-falign-jumps=8'],
 'nocross':['-O2','-fno-crossjumping'],
 'nocross-j1':['-O2','-fno-crossjumping','-falign-jumps=1'],
 'nocross-j4':['-O2','-fno-crossjumping','-falign-jumps=4'],
 'noreorder':['-O2','-fno-reorder-blocks'],
 'no-thread':['-O2','-fno-thread-jumps'],
 'no-csefollow':['-O2','-fno-cse-follow-jumps'],
 'j4l2':['-O2','-falign-jumps=4','-falign-labels=2'],
}
T=Path('/tmp/vuc-binutils215/stmt-list-probability-crossjump-oct15c');T.mkdir(parents=True,exist_ok=True)
rows=[];sizes=collections.Counter();best=None
for bits,(name,opts) in itertools.product(itertools.product((0,1),repeat=3),flags.items()):
 tag=''.join(map(str,bits))+'-'+name
 code=base
 for i,ex in enumerate(old):
  f=ex.replace(', 1)',f', {bits[i]})').replace(', 0)',f', {bits[i]})')
  code=code.replace(ex,f)
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(code)
 p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:rows.append(dict(tag=tag,error=p.stderr[-130:]));continue
 try:b,r=m.prepare_named_function(obj.read_bytes(),conf['function'],conf['relocations'],a)
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)));continue
 match=sum(x==y for x,y in zip(ref,b));prefix=next((i for i,(x,y) in enumerate(zip(ref,b)) if x!=y),min(len(ref),len(b)))
 row=dict(tag=tag,length=len(b),relocations=r,match=match,prefix=prefix)
 rows.append(row);sizes[len(b)]+=1
 if len(b)==119 and r==3 and (best is None or (match,prefix)>(best['match'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(code);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(opts)+'\n')
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best=best,top=sorted((r for r in rows if 'match' in r),key=lambda r:(r['match'],r['prefix']),reverse=True)[:13]),indent=2))
