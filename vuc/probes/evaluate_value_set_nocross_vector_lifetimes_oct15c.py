#!/usr/bin/env python3
"""VUC original Value::Set: preserve three sequential boolean c_str calls
and original 64-byte stack scratch; change natural vector parsing lifetimes
toward original 455-byte function with all 15 genuine reference sites.
"""
from pathlib import Path
import itertools,importlib.util,subprocess,json,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/'vuc/config/value_set_constval_partial_oct15b.json').read_text())
raw,elf=m.decomp.load();s=next(x for x in elf['sections'] if x['name']=='.text');a=int(cfg['address'],0);ref=raw[s['offset']+a-s['address']:s['offset']+a-s['address']+455]
base=(R/'vuc/probes/ValueConstValSetPartialOct15b.cpp').read_text().replace('#include "../src/MLGameString.h"','#include "'+str(R/'vuc/src/MLGameString.h')+'"').replace('char scratch[100]','char scratch[64]')
base=base.replace('        const char *second = expression->source.c_str();\n','').replace('std::strcmp(second, "false")','std::strcmp(expression->source.c_str(), "false")')
vec=base[base.index('    case 4: {'):base.rindex('    }\n}')]
assert 'char scratch[64]' in vec
mods={
 'base':lambda s:s,
 'lenint':lambda s:s.replace('unsigned length = comma - text;', 'int length = comma - text;'),
 'lensize':lambda s:s.replace('unsigned length = comma - text;', 'std::size_t length = comma - text;'),
 'lenconst':lambda s:s.replace('unsigned length = comma - text;', 'const unsigned length = comma - text;'),
 'nextbeforecopy':lambda s:s.replace('            std::memcpy(scratch, text, length);','            const char *next = comma + 1;\n            std::memcpy(scratch, text, length);').replace('            text = comma + 1;', '            text = next;'),
 'nextbeforeparse':lambda s:s.replace('            values[i] = (float)__strtod_internal(scratch, 0, 0);','            const char *next = comma + 1;\n            values[i] = (float)__strtod_internal(scratch, 0, 0);').replace('            text = comma + 1;', '            text = next;'),
 'advanceearly':lambda s:s.replace("            scratch[length] = '\\0';", "            scratch[length] = '\\0';\n            text = comma + 1;").replace('            text = comma + 1;\n        }','        }'),
 'constcomma':lambda s:s.replace('const char *comma =', 'const char *const comma ='),
 'unsignedloop':lambda s:s.replace('for (int i = 0; i < 3; ++i)', 'for (unsigned i = 0; i < 3; ++i)'),
 'whileloop':lambda s:s.replace('for (int i = 0; i < 3; ++i) {','int i = 0; while (i < 3) {').replace('            text = comma + 1;', '            text = comma + 1;\n            ++i;'),
 'dowhile':lambda s:s.replace('for (int i = 0; i < 3; ++i) {','int i = 0; do {').replace('            text = comma + 1;\n        }','            text = comma + 1;\n            ++i;\n        } while (i < 3);'),
 'bufoffset':lambda s:s.replace('std::memcpy(scratch, text, length);','std::memcpy(&scratch[0], text, length);'),
 'memcpylen':lambda s:s.replace('std::memcpy(scratch, text, length);','std::memcpy(scratch, text, (unsigned)(comma - text));'),
 'terminateptr':lambda s:s.replace("scratch[length] = '\\0';","*(scratch + length) = '\\0';"),
 'terminateexpr':lambda s:s.replace("scratch[length] = '\\0';","scratch[comma - text] = '\\0';"),
 'lastdouble':lambda s:s.replace('values[3] = (float)__strtod_internal(text, 0, 0);','const double last = __strtod_internal(text, 0, 0); values[3] = (float)last;'),
 'spacedlocal':lambda s:s.replace('const char *text = expression->source.c_str();','const MLGameString &source = expression->source;\n        const char *text = source.c_str();'),
}
flags={
 'j1':['-O2','-frename-registers','-falign-jumps=1','-fno-crossjumping'],
 'j2':['-O2','-frename-registers','-falign-jumps=2','-fno-crossjumping'],
 'j4':['-O2','-frename-registers','-falign-jumps=4','-fno-crossjumping'],
 'j4l2':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=2','-fno-crossjumping'],
 'plain':['-O2','-falign-jumps=4','-fno-crossjumping'],
}
T=Path('/tmp/vuc-binutils215/value-set-nocross-vector-lifetimes-oct15c');T.mkdir(parents=True,exist_ok=True)
rows=[];counts=collections.Counter();best=None
for (name,modify),(fl,opts),snapshot in itertools.product(mods.items(),flags.items(),('jan25','dec06')):
 tag=f'{name}-{fl}-{snapshot}'
 code=base.replace(vec,modify(vec))
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(code)
 p=subprocess.run(['/tmp/vuc-binutils215/g++-'+snapshot+'-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:rows.append(dict(tag=tag,error=p.stderr[-130:]));continue
 try:b,rel=m.prepare_named_function(obj.read_bytes(),cfg['function'],cfg['relocations'],a,literal_ranges=cfg['relocation_literals'])
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)));continue
 score=sum(x==y for x,y in zip(b,ref));prefix=next((i for i,(x,y) in enumerate(zip(b,ref)) if x!=y),min(len(b),len(ref)))
 row=dict(tag=tag,length=len(b),relocations=rel,match=score,prefix=prefix);rows.append(row);counts[len(b)]+=1
 if len(b)==455 and rel==15 and (best is None or (score,prefix)>(best['match'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(code);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(opts)+'\n')
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(counts),best=best,top=sorted((r for r in rows if 'match' in r),key=lambda r:(r['match'],r['prefix']),reverse=True)[:16]),indent=2))
