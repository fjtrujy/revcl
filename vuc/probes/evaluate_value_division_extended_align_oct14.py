#!/usr/bin/env python3
"""Independent original-length Value division historical GCC alignment controls.

The retained partial includes true Value +,-,* compiler source predecessors.
This sweeps real label/jump/loop alignment, strictly checks original 235-byte
size and original TypeConvert relocation; no machine-code data is patched.
"""
from pathlib import Path
import itertools,importlib.util,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/'vuc/config/value_division_partial_oct13.json').read_text())
raw,elf=m.decomp.load();sec=next(x for x in elf['sections'] if x['name']=='.text')
address=int(cfg['address'],0);orig=raw[sec['offset']+address-sec['address']:sec['offset']+address-sec['address']+cfg['size']]
T=Path('/tmp/vuc-binutils215/value-division-extended-align-batch');T.mkdir(parents=True,exist_ok=True)
rows=[];sizes=collections.Counter();best=None
for jumps,labels,loops,reorder in itertools.product((1,4,8,16),(1,4,8,16),(1,4),(True,False)):
 name=f'j{jumps}-l{labels}-loop{loops}-r{int(reorder)}'
 flags=['-O2','-frename-registers',f'-falign-jumps={jumps}',f'-falign-labels={labels}',f'-falign-loops={loops}']
 if not reorder: flags+=['-fno-reorder-blocks']
 obj=T/(name+'.o')
 cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(R/cfg['source']),'-o',str(obj)],capture_output=True,text=True)
 if cp.returncode:rows.append(dict(name=name,error=cp.stderr[-180:]));continue
 try:body,rel=m.prepare_named_function(obj.read_bytes(),cfg['function'],cfg['relocations'],address)
 except ValueError as e:rows.append(dict(name=name,error=str(e)));continue
 overlap=sum(a==b for a,b in zip(body,orig))
 prefix=next((i for i,(a,b) in enumerate(zip(body,orig)) if a!=b),min(len(body),len(orig)))
 row=dict(name=name,length=len(body),relocations=rel,overlap=overlap,prefix=prefix)
 rows.append(row);sizes[len(body)]+=1
 if len(body)==cfg['size'] and rel==1 and (best is None or (overlap,prefix)>(best['overlap'],best['prefix'])):
  best=row;(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(flags)+'\n')
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),lengths=dict(sizes),best_original_length=best,top=sorted((r for r in rows if 'overlap' in r),key=lambda r:(r['overlap'],r['prefix']),reverse=True)[:10]),indent=2))
