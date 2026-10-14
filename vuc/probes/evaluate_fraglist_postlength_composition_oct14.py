#!/usr/bin/env python3
"""VUC ASTNodeList::Add postlength (+5 bytes) with genuine source lifetimes.

Source experiment only: the original inlined ASTNodeList append updates a
length that might have been expressed as ++length. Combine with alternate
fragment construction, index update, and return/result lifetimes to test
whether source-level code compensates the original frame/CFG without padding.
"""
from pathlib import Path
import itertools,subprocess,importlib.util,json,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/'vuc/config/translate_frag_list_string_oct14_probe.json').read_text())
raw,elf=m.decomp.load();sec=next(s for s in elf['sections'] if s['name']=='.text')
addr=int(cfg['address'],0);original=raw[sec['offset']+addr-sec['address']:sec['offset']+addr-sec['address']+306]
base=(R/'vuc/probes/translate_frag_list_string_partial_oct14.cpp').read_text()
base=base.replace('"../src/MLGameString.h"','"'+str(R/'vuc/src/MLGameString.h')+'"')
frag='const MLGameString &name = input.entries[index];\n        IR::Fragment_Data *fragment = new IR::Fragment_Data(name);'
idx='int index = i;'
growth='length = old_size + 1;'
copy='__builtin_memcpy(entries, old_entries, length * sizeof(T));'
assert all(v in base for v in (frag,idx,growth,copy))
fragment_variants={
 'reference':frag,
 'direct':'IR::Fragment_Data *fragment = new IR::Fragment_Data(input.entries[index]);',
 'pointer':'const MLGameString *name = input.entries + index;\n        IR::Fragment_Data *fragment = new IR::Fragment_Data(*name);',
}
index_variants={
 'before-add':('int index = i;','++i;\n        output.Add(fragment);'),
 'post-index':('int index = i++;','output.Add(fragment);'),
 'after-add':('int index = i;','output.Add(fragment);\n        ++i;'),
 'before-fragment':('int index = i;\n        ++i;','output.Add(fragment);'),
}
copy_variants={
 'memcpy':copy,
 'memmove':'__builtin_memmove(entries, old_entries, length * sizeof(T));',
 'old_size':'__builtin_memcpy(entries, old_entries, old_size * sizeof(T));',
}
flags={'j1':['-O2','-frename-registers','-falign-jumps=1'],
       'j4':['-O2','-frename-registers','-falign-jumps=4'],
       'j4-nr':['-O2','-frename-registers','-falign-jumps=4','-fno-reorder-blocks']}
T=Path('/tmp/vuc-binutils215/fraglist-postlength-composition-oct14');T.mkdir(exist_ok=True,parents=True)
records=[];sizes=collections.Counter();best=None
for (f,fragcode),(i,(pre,post)),(c,copycode),flag in itertools.product(
 fragment_variants.items(),index_variants.items(),copy_variants.items(),flags):
 tag=f'{f}-{i}-{c}-{flag}'
 src=base.replace(growth,'++length;').replace(copy,copycode).replace(frag,fragcode)
 src=src.replace(idx,pre)
 src=src.replace('++i;\n        output.Add(fragment);',post)
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o')
 cpp.write_text(src)
 cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags[flag],'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if cp.returncode:records.append(dict(tag=tag,error=cp.stderr[-150:]));continue
 try:code,rel=m.prepare_named_function(obj.read_bytes(),cfg['function'],cfg['relocations'],addr)
 except ValueError as e:records.append(dict(tag=tag,error=str(e)));continue
 score=sum(a==b for a,b in zip(code,original));sizes[len(code)]+=1
 prefix=next((n for n,(a,b) in enumerate(zip(code,original)) if a!=b),min(len(code),len(original)))
 row=dict(tag=tag,length=len(code),relocations=rel,overlap=score,prefix=prefix)
 records.append(row)
 if len(code)==306 and rel==6 and (best is None or (score,prefix)>(best['overlap'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(src);(T/'best.o').write_bytes(obj.read_bytes())
(T/'results.json').write_text(json.dumps(records,indent=2)+'\n')
print(json.dumps(dict(attempted=len(records),sizes=dict(sizes),best_original_length=best,top=sorted((r for r in records if 'overlap'in r),key=lambda r:(r['overlap'],r['prefix']),reverse=True)[:12]),indent=2))
