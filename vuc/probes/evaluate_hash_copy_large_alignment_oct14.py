#!/usr/bin/env python3
"""Original-era GCC alignment controls for an authentic VUC hash deep copy.

Original constructor 0x0804f420 = 166 bytes with four EH/copy relocations.
Natural Node copy source currently 162 bytes. This tests only genuine compiler
flags for historic aligned branches, not synthetic NOPs or fake relocations.
"""
from pathlib import Path
import itertools,importlib.util,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/'vuc/config/hash_copy_constructor_probe.json').read_text())
raw,elf=m.decomp.load();sec=next(s for s in elf['sections'] if s['name']=='.text')
addr=int(cfg['address'],0);original=raw[sec['offset']+addr-sec['address']:sec['offset']+addr-sec['address']+cfg['size']]
T=Path('/tmp/vuc-binutils215/hash-copy-large-alignment-batch');T.mkdir(parents=True,exist_ok=True)
rows=[];counts=collections.Counter();best=None
for jumps,labels,loops,snapshot in itertools.product((1,4,8,16),(1,4,8,16),(1,4),('jan25','dec06')):
 name=f'j{jumps}-l{labels}-loop{loops}-{snapshot}'
 flags=['-O2','-frename-registers',f'-falign-jumps={jumps}',f'-falign-labels={labels}',f'-falign-loops={loops}']
 obj=T/(name+'.o')
 cp=subprocess.run(['/tmp/vuc-binutils215/g++-'+snapshot+'-as215',*flags,'-c',str(R/cfg['source']),'-o',str(obj)],capture_output=True,text=True)
 if cp.returncode:rows.append(dict(name=name,error=cp.stderr[-200:]));continue
 try:body,rel=m.prepare_named_function(obj.read_bytes(),cfg['function'],cfg['relocations'],addr)
 except ValueError as e:rows.append(dict(name=name,error=str(e)));continue
 score=sum(x==y for x,y in zip(body,original))
 prefix=next((i for i,(x,y) in enumerate(zip(body,original)) if x!=y),min(len(body),len(original)))
 rec=dict(name=name,length=len(body),relocations=rel,overlap=score,prefix=prefix)
 rows.append(rec);counts[len(body)]+=1
 if len(body)==166 and rel==4 and (best is None or (score,prefix)>(best['overlap'],best['prefix'])):
  best=rec;(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(flags)+'\n')
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(counts),best_original_length=best,top=sorted((r for r in rows if 'overlap' in r),key=lambda r:(r['overlap'],r['prefix']),reverse=True)[:10]),indent=2))
