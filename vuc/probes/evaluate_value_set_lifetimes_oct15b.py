#!/usr/bin/env python3
"""VUC original AST constant setter: genuine C++03 pointer/lifetime controls.
Original-length 455-byte and 13 call/string relocations are mandatory.
Potential improvement is strictly measured by original-offset byte overlap.
"""
from pathlib import Path
import itertools,importlib.util,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=R/'vuc/config/value_set_constval_partial_oct15b.json'
base=(R/'vuc/probes/ValueConstValSetPartialOct15b.cpp').read_text().replace('#include "../src/MLGameString.h"','#include "'+str(R/'vuc/src/MLGameString.h')+'"')
bool0='''        const char *first = expression->source.c_str();
        const char *second = expression->source.c_str();
        if (std::strcmp(first, "true") == 0) bits = 1;
        else if (std::strcmp(second, "false") == 0) bits = 0;
        else bits = __strtol_internal(expression->source.c_str(), 0, 10, 0);'''
assert bool0 in base
boolforms={
 'saved':bool0,
 'firstconst':bool0.replace('const char *first', 'const char *const first'),
 'secondconst':bool0.replace('const char *second','const char *const second'),
 'bothconst':bool0.replace('const char *first','const char *const first').replace('const char *second','const char *const second'),
 'firstptr':bool0.replace('const char *first =', 'const char *const first ='),
 'flaglocal':bool0.replace('if (std::strcmp(first, "true") == 0) bits = 1;','const bool yes=std::strcmp(first, "true") == 0;\n        if (yes) bits = 1;'),
 'nested':bool0.replace('else if (std::strcmp(second, "false") == 0) bits = 0;\n        else bits = __strtol_internal(expression->source.c_str(), 0, 10, 0);','else { if (std::strcmp(second, "false") == 0) bits=0;\n               else bits=__strtol_internal(expression->source.c_str(), 0, 10, 0); }'),
 'second-after-first':bool0.replace('        const char *second = expression->source.c_str();\n','').replace('else if (std::strcmp(second, "false") == 0) bits = 0;', 'else if (std::strcmp(expression->source.c_str(), "false") == 0) bits = 0;'),
}
vec0='''        const char *text = expression->source.c_str();
        char scratch[100];
        for (int i = 0; i < 3; ++i) {
            const char *comma = std::strrchr(text, ',');
            unsigned length = comma - text;
            std::memcpy(scratch, text, length);
            scratch[length] = '\\0';
            values[i] = (float)__strtod_internal(scratch, 0, 0);
            text = comma + 1;
        }
        values[3] = (float)__strtod_internal(text, 0, 0);'''
assert vec0 in base
vforms={
 'saved':vec0,
 'next-early':vec0.replace('            std::memcpy(scratch, text, length);','            const char *next=comma+1;\n            std::memcpy(scratch, text, length);').replace('            text = comma + 1;','            text = next;'),
 'next-before-parse':vec0.replace("            scratch[length] = '\\0';","            scratch[length] = '\\0';\n            text = comma + 1;").replace('            text = comma + 1;\n        }','        }'),
 'signedlen':vec0.replace('unsigned length = comma - text;','int length = comma - text;'),
 'ptrdiff':vec0.replace('unsigned length = comma - text;','std::ptrdiff_t length = comma - text;'),
 'scratchptr':vec0.replace('std::memcpy(scratch, text, length);','std::memcpy(&scratch[0], text, length);'),
 'while':vec0.replace('for (int i = 0; i < 3; ++i) {','int i = 0;\n        while (i < 3) {').replace('            text = comma + 1;','            text = comma + 1;\n            ++i;'),
 'do':vec0.replace('for (int i = 0; i < 3; ++i) {','int i = 0;\n        do {').replace('            text = comma + 1;\n        }','            text = comma + 1;\n            ++i;\n        } while (i < 3);'),
 'unsignedindex':vec0.replace('for (int i = 0; i < 3; ++i)','for (unsigned i = 0; i < 3; ++i)'),
 'lenlocal':vec0.replace('unsigned length = comma - text;','const unsigned length = comma - text;'),
 'constscratch':vec0.replace('char scratch[100];','char scratch[100] = {0};'),
}
flags={
 'saved':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=2','-falign-loops=1','-fno-reorder-blocks'],
 'j4l2':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=2'],
 'j1':['-O2','-frename-registers','-falign-jumps=1'],
 'plain':['-O2','-falign-jumps=4','-falign-labels=2','-fno-reorder-blocks'],
}
T=Path('/tmp/vuc-binutils215/vuc-value-set-lifetimes-oct15b');T.mkdir(parents=True,exist_ok=True)
results=[];lengths=collections.Counter();best=None
for (bn,b),(vn,v),(fn,opts) in itertools.product(boolforms.items(),vforms.items(),flags.items()):
 tag=f'{bn}-{vn}-{fn}';cpp=T/(tag+'.cpp');obj=T/(tag+'.o')
 cpp.write_text(base.replace(bool0,b).replace(vec0,v))
 r=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if r.returncode:results.append(dict(tag=tag,error=r.stderr[-130:]));continue
 symbol=next((line for line in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines() if line.endswith('_ZN5Value3SetEPK15ASTConstValExpr')),None)
 n=int(symbol.split()[1],16) if symbol else 0;lengths[n]+=1
 if n!=455:results.append(dict(tag=tag,length=n));continue
 try:d=m.run(obj,cfg)
 except ValueError as e:results.append(dict(tag=tag,length=n,error=str(e)[:120]));continue
 row=dict(tag=tag,length=n,match=d['exact_bytes'],relocations=d['relocations'],literal_bytes=d['verified_literal_bytes'],prefix=next(iter(d['different_offsets']),n))
 results.append(row)
 if best is None or (row['match'],row['prefix'])>(best['match'],best['prefix']):
  best=row;(T/'best.cpp').write_text(cpp.read_text());(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(opts)+'\n')
(T/'results.json').write_text(json.dumps(results,indent=2)+'\n')
print(json.dumps(dict(attempted=len(results),sizes=dict(lengths),best=best,top=sorted((r for r in results if 'match' in r),key=lambda r:(r['match'],r['prefix']),reverse=True)[:15]),indent=2))
