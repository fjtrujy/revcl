#!/usr/bin/env python3
"""Original VUC Value AST constant setter: bounded zero-terminated string tests.
Both strcmp and memcmp(..., length including NUL) have identical semantics
for null-terminated original boolean string literals. Probe genuine C++03
source alternatives and historical code-generation pass flags.
"""
from pathlib import Path
import itertools,subprocess,json,importlib.util,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=R/'vuc/config/value_set_constval_partial_oct15b.json'
base=(R/'vuc/probes/ValueConstValSetPartialOct15b.cpp').read_text().replace('#include "../src/MLGameString.h"','#include "'+str(R/'vuc/src/MLGameString.h')+'"').replace('char scratch[100]','char scratch[64]')
form={
 'strcmp':['std::strcmp(first, "true")','std::strcmp(second, "false")'],
 'memcmp':['std::memcmp(first, "true", 5)','std::memcmp(second, "false", 6)'],
 'builtin':['__builtin_strcmp(first, "true")','__builtin_strcmp(second, "false")'],
 'builtinmem':['__builtin_memcmp(first, "true", 5)','__builtin_memcmp(second, "false", 6)'],
}
bools={
 'two':base,
 'fetch-inline':base.replace('std::strcmp(first, "true")','std::strcmp(expression->source.c_str(), "true")').replace('std::strcmp(second, "false")','std::strcmp(expression->source.c_str(), "false")'),
 'second-later':base.replace('        const char *second = expression->source.c_str();\n','').replace('        else if (std::strcmp(second, "false") == 0)', '        else if (std::strcmp(expression->source.c_str(), "false") == 0)'),
 'separate-cstr':base.replace('        const char *second = expression->source.c_str();','        const char *second = expression->source.c_str();\n        const char *third = expression->source.c_str();').replace('else bits = __strtol_internal(expression->source.c_str(), 0, 10, 0);','else bits = __strtol_internal(third, 0, 10, 0);'),
 'bool-branch':base.replace('else bits = __strtol_internal(expression->source.c_str(), 0, 10, 0);','else { const char *s=expression->source.c_str(); bits = __strtol_internal(s, 0, 10, 0); }'),
}
flags={
 'j1-nocross':['-O2','-frename-registers','-falign-jumps=1','-fno-crossjumping'],
 'j4-nocross':['-O2','-frename-registers','-falign-jumps=4','-fno-crossjumping'],
 'j4l2-nocross':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=2','-fno-crossjumping'],
 'j4l4-nocross':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=4','-fno-crossjumping'],
 'j4-nocrossnoreorder':['-O2','-frename-registers','-falign-jumps=4','-fno-crossjumping','-fno-reorder-blocks'],
 'plain-nocross':['-O2','-falign-jumps=4','-fno-crossjumping'],
 'j4-nocrossnosched':['-O2','-frename-registers','-falign-jumps=4','-fno-crossjumping','-fno-schedule-insns2'],
}

T=Path('/tmp/vuc-binutils215/value-set-buffer64-boolcompare-nocross-oct15c');T.mkdir(exist_ok=True,parents=True)
results=[];sizes=collections.Counter();best=None
for (first,fcomp),(second,scomp),(name,src),(fn,opts) in itertools.product(form.items(),form.items(),bools.items(),flags.items()):
 tag=f'{first}-{second}-{name}-{fn}'
 code=src.replace('std::strcmp(first, "true")',fcomp[0]).replace('std::strcmp(second, "false")',scomp[1])
 if 'std::strcmp(' not in code and 'std::memcmp(' not in code and '__builtin' not in code:continue
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(code)
 r=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if r.returncode:results.append(dict(tag=tag,error=r.stderr[-140:]));continue
 sy=next((line for line in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines() if line.endswith('_ZN5Value3SetEPK15ASTConstValExpr')),None)
 n=int(sy.split()[1],16) if sy else 0;sizes[n]+=1
 if n!=455:results.append(dict(tag=tag,length=n));continue
 try:d=m.run(obj,cfg)
 except ValueError as e:results.append(dict(tag=tag,length=n,error=str(e)[:150]));continue
 row=dict(tag=tag,length=n,match=d['exact_bytes'],relocations=d['relocations'],literal_bytes=d['verified_literal_bytes'],prefix=next(iter(d['different_offsets']),n))
 results.append(row)
 if best is None or (row['match'],row['prefix'])>(best['match'],best['prefix']):
  best=row;(T/'best.cpp').write_text(code);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(opts)+'\n')
(T/'results.json').write_text(json.dumps(results,indent=2)+'\n')
print(json.dumps(dict(attempted=len(results),sizes=dict(sizes),best=best,top=sorted((r for r in results if 'match' in r),key=lambda r:(r['match'],r['prefix']),reverse=True)[:15]),indent=2))
