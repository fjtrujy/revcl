#!/usr/bin/env python3
"""Check authentic Value logical AND switch-case lexical order and GCC layout.

All variants have the same C++03 switch semantics. Compare full original-size
body and original relocated TypeConvert call, no artificial alignment bytes.
"""
from pathlib import Path
import itertools,importlib.util,subprocess,json,collections
R=Path(__file__).resolve().parents[2];sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/'vuc/config/value_logical_and_partial_oct14_probe.json').read_text())
raw,elf=m.decomp.load();sec=next(x for x in elf['sections'] if x['name']=='.text')
addr=int(cfg['address'],0);ref=raw[sec['offset']+addr-sec['address']:sec['offset']+addr-sec['address']+325]
src=(R/cfg['source']).read_text()
start=src.index('   case 1: case 2:')
end=src.index('\n  }\n }\n return result;',start)
arms={
 'int':'case 1: case 2: result.bits=(result.bits && temporary.bits);break;',
 'scalar':'case 3: result.bits=(result.values[0] != 0.0f && temporary.values[0] != 0.0f);break;',
 'vector': 'case 4: result.bits='+src.split('case 4: result.bits=',1)[1].split(';break;',1)[0]+';break;',
}
T=Path('/tmp/vuc-binutils215/value-and-case-lexical-order');T.mkdir(parents=True,exist_ok=True)
rows=[];best=None;sizes=collections.Counter()
for order,jump,label,reorder in itertools.product(itertools.permutations(arms), (1,8,16),(1,8,16),(True,False)):
 tag='-'.join(order)+f'-j{jump}-l{label}-r{int(reorder)}'
 code=src[:start]+'   '+'\n   '.join(arms[k] for k in order)+src[end:]
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(code)
 flags=['-O2','-frename-registers','-ffast-math',f'-falign-jumps={jump}',f'-falign-labels={label}','-falign-loops=1']
 if not reorder:flags+=['-fno-reorder-blocks']
 cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if cp.returncode:rows.append(dict(tag=tag,error=cp.stderr[-120:]));continue
 try:body,rel=m.prepare_named_function(obj.read_bytes(),cfg['function'],cfg['relocations'],addr)
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)));continue
 score=sum(a==b for a,b in zip(body,ref))
 prefix=next((i for i,(a,b) in enumerate(zip(body,ref)) if a!=b),min(len(body),len(ref)))
 record=dict(tag=tag,length=len(body),relocations=rel,overlap=score,prefix=prefix)
 rows.append(record);sizes[len(body)]+=1
 if len(body)==325 and rel==1 and (best is None or (score,prefix)>(best['overlap'],best['prefix'])):
  best=record;(T/'best.cpp').write_text(code);(T/'best.flags').write_text(' '.join(flags)+'\n');(T/'best.o').write_bytes(obj.read_bytes())
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best=best,top=sorted((x for x in rows if 'overlap' in x),key=lambda r:(r['overlap'],r['prefix']),reverse=True)[:10]),indent=2))
