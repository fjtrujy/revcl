#!/usr/bin/env python3
"""VUC Value::Set 213/459 original overlap: ordinary C++03 register hints and lifetimes.
Original keeps vector advance pointer in ESI across calls; reconstructed GCC
spills it on stack. Test register/const/reference source hypotheses, never
explicit x86 register bindings, assembler, fabricated alignment padding.
"""
from pathlib import Path
import importlib.util,itertools,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=R/'vuc/config/value_set_constval_partial_oct15b.json'
cfg=json.loads(cfg.read_text())
base=(R/'vuc/probes/ValueConstValSetPartialOct15b.cpp').read_text().replace('#include "../src/MLGameString.h"','#include "'+str(R/'vuc/src/MLGameString.h')+'"').replace('char scratch[100]','char scratch[64]')
base=base.replace('        const char *second = expression->source.c_str();\n','').replace('std::strcmp(second, "false")','std::strcmp(expression->source.c_str(), "false")')
base=base.replace('            std::memcpy(scratch, text, length);','            const char *next = comma + 1;\n            std::memcpy(scratch, text, length);').replace('            text = comma + 1;','            text = next;')
def trans(name,source):
 if name=='baseline':return source
 if name=='register-text':return source.replace('        const char *text = expression->source.c_str();','        register const char *text = expression->source.c_str();')
 if name=='register-next':return source.replace('            const char *next = comma + 1;','            register const char *next = comma + 1;')
 if name=='register-both':return source.replace('        const char *text = expression->source.c_str();','        register const char *text = expression->source.c_str();').replace('            const char *next = comma + 1;','            register const char *next = comma + 1;')
 if name=='register-comma':return source.replace("            const char *comma = std::strrchr(text, ',');","            register const char *comma = std::strrchr(text, ',');")
 if name=='register-index':return source.replace('for (int i = 0; i < 3; ++i)','for (register int i = 0; i < 3; ++i)')
 if name=='register-length':return source.replace('unsigned length = comma - text;','register unsigned length = comma - text;')
 if name=='register-text-index':return trans('register-text',trans('register-index',source))
 if name=='register-text-comma':return trans('register-text',trans('register-comma',source))
 if name=='register-next-index':return trans('register-next',trans('register-index',source))
 if name=='register-next-comma':return trans('register-next',trans('register-comma',source))
 if name=='pointer-reference':return source.replace('        const char *text = expression->source.c_str();','        const MLGameString &string = expression->source;\n        const char *text = string.c_str();')
 if name=='pointer-owner':return source.replace('        const char *text = expression->source.c_str();','        const MLGameString *string = &expression->source;\n        const char *text = string->c_str();')
 if name=='mutable-text':return source.replace('        const char *text = expression->source.c_str();','        char *text = const_cast<char *>(expression->source.c_str());').replace('            const char *next = comma + 1;','            char *next = const_cast<char *>(comma + 1);')
 if name=='register-mutable-text':return trans('mutable-text',trans('register-text',source))
 if name=='const-next':return source.replace('const char *next = comma + 1;','const char *const next = comma + 1;')
 if name=='saved-current':return source.replace('const char *next = comma + 1;','const char *current=text;\n            const char *next = comma+1;').replace('std::memcpy(scratch, text, length);','std::memcpy(scratch, current, length);')
 if name=='saved-srcptr':return source.replace('        const char *text = expression->source.c_str();','        const char *text = expression->source.c_str();\n        const char *saved=text;').replace('            std::memcpy(scratch, text, length);','            std::memcpy(scratch, saved, length);').replace('            text = next;','            text = next;\n            saved = text;')
 if name=='next-before-copy':return source.replace('            const char *next = comma + 1;\n            std::memcpy(scratch, text, length);','            const char *next = comma + 1;\n            const char *original = text;\n            std::memcpy(scratch, original, length);')
 if name=='store-inline':return source.replace('            const char *next = comma + 1;\n            std::memcpy(scratch, text, length);','            const char *next = comma + 1;\n            std::memcpy(&scratch[0], text, length);')
 if name=='register-scratch':return source.replace('        char scratch[64];','        char scratch[64];\n        register char *const output = scratch;').replace('std::memcpy(scratch,','std::memcpy(output,').replace('scratch[length]','output[length]').replace('__strtod_internal(scratch,','__strtod_internal(output,')
 raise ValueError(name)
names=('baseline','register-text','register-next','register-both','register-comma','register-index','register-length','register-text-index','register-text-comma','register-next-index','register-next-comma','pointer-reference','pointer-owner','mutable-text','register-mutable-text','const-next','saved-current','saved-srcptr','next-before-copy','store-inline','register-scratch')
flags={
 'j1':['-O2','-frename-registers','-falign-jumps=1','-fno-crossjumping'],
 'j4':['-O2','-frename-registers','-falign-jumps=4','-fno-crossjumping'],
 'j4-no-regmove':['-O2','-frename-registers','-falign-jumps=4','-fno-crossjumping','-fno-regmove'],
 'plain':['-O2','-falign-jumps=4','-fno-crossjumping'],
}
T=Path('/tmp/vuc-binutils215/value-set-459-pointer-lifetimes-oct15c');T.mkdir(parents=True,exist_ok=True)
rows=[];sizes=collections.Counter();best=None
for name,(fk,opts),snapshot in itertools.product(names,flags.items(),('jan25','dec06')):
 tag=f'{name}-{fk}-{snapshot}';cpp=T/(tag+'.cpp');obj=T/(tag+'.o')
 cpp.write_text(trans(name,base))
 p=subprocess.run(['/tmp/vuc-binutils215/g++-'+snapshot+'-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:rows.append(dict(tag=tag,error=p.stderr[-130:]));continue
 try:body,r=m.prepare_named_function(obj.read_bytes(),cfg['function'],cfg['relocations'],int(cfg['address'],0),literal_ranges=cfg['relocation_literals'])
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)));continue
 raw,elf=m.decomp.load();s=next(s for s in elf['sections'] if s['name']=='.text');a=int(cfg['address'],0);ref=raw[s['offset']+a-s['address']:s['offset']+a-s['address']+455]
 score=sum(x==y for x,y in zip(ref,body));prefix=next((i for i,(x,y) in enumerate(zip(ref,body)) if x!=y),min(len(ref),len(body)))
 row=dict(tag=tag,length=len(body),relocations=r,match=score,prefix=prefix)
 rows.append(row);sizes[len(body)]+=1
 if len(body)==455 and r==15 and (best is None or (score,prefix)>(best['match'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(cpp.read_text());(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(opts)+'\n')
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best=best,top=sorted((r for r in rows if 'match' in r),key=lambda r:(r['match'],r['prefix']),reverse=True)[:20]),indent=2))
