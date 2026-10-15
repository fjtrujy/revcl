#!/usr/bin/env python3
"""VUC Value::Set(ASTConstValExpr) authentic ABI source hypotheses.
Ground original AST offsets +12..+28 and glibc symbols from original 455-byte
function; attempt correct source lifetimes/loop layout. Only original-sized
candidate with all real relocations/literals may be selected as partial.
"""
from pathlib import Path
import importlib.util,itertools,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
base=(R/'vuc/probes/value_constval_set_initial_oct15b.cpp').read_text().replace(
    '#include "../src/MLGameString.h"',
    '#include "'+str(R/'vuc/src/MLGameString.h')+'"')
cfg=R/'vuc/config/value_set_constval_investigation_oct15b.json'
section=base[base.index('    case 2: {'):base.index('    case 3:')]
vector=base[base.index('    case 4: {'):]
boolforms={
 'direct':'''    case 2: {
        if (std::strcmp(expression->source.c_str(), "true") == 0) bits = 1;
        else if (std::strcmp(expression->source.c_str(), "false") == 0) bits = 0;
        else bits = __strtol_internal(expression->source.c_str(), 0, 10, 0);
        break;
    }
''',
 'cached':'''    case 2: {
        const char *text = expression->source.c_str();
        if (std::strcmp(text, "true") == 0) bits = 1;
        else if (std::strcmp(text, "false") == 0) bits = 0;
        else bits = __strtol_internal(expression->source.c_str(), 0, 10, 0);
        break;
    }
''',
 'cached-first':'''    case 2: {
        const char *text = expression->source.c_str();
        if (std::strcmp(text, "true") == 0) bits = 1;
        else if (std::strcmp(expression->source.c_str(), "false") == 0) bits = 0;
        else bits = __strtol_internal(expression->source.c_str(), 0, 10, 0);
        break;
    }
''',
 'cached-second':'''    case 2: {
        if (std::strcmp(expression->source.c_str(), "true") == 0) bits = 1;
        else { const char *text=expression->source.c_str();
               if (std::strcmp(text, "false") == 0) bits = 0;
               else bits = __strtol_internal(expression->source.c_str(), 0, 10, 0); }
        break;
    }
''',
 'cached-fallback':'''    case 2: {
        const char *text=expression->source.c_str();
        if (std::strcmp(expression->source.c_str(), "true") == 0) bits = 1;
        else if (std::strcmp(expression->source.c_str(), "false") == 0) bits = 0;
        else bits = __strtol_internal(text, 0, 10, 0);
        break;
    }
''',
 'new-locals':'''    case 2: {
        const char *first = expression->source.c_str();
        const bool yes = std::strcmp(first, "true") == 0;
        if (yes) bits = 1;
        else { const char *second = expression->source.c_str();
               const bool no = std::strcmp(second, "false") == 0;
               if (no) bits = 0;
               else bits = __strtol_internal(expression->source.c_str(), 0, 10, 0); }
        break;
    }
''',
 'two-sources':'''    case 2: {
        const char *first = expression->source.c_str();
        const char *second = expression->source.c_str();
        if (std::strcmp(first, "true") == 0) bits = 1;
        else if (std::strcmp(second, "false") == 0) bits = 0;
        else bits = __strtol_internal(expression->source.c_str(), 0, 10, 0);
        break;
    }
''',
}
assert section==boolforms['direct']
loopforms={
 'base':vector,
 'advance-before-memcpy':vector.replace('            std::memcpy(scratch, text, length);','            const char *next=comma+1;\n            std::memcpy(scratch, text, length);').replace('            text = comma + 1;','            text = next;'),
 'advance-before-strtod':vector.replace("            scratch[length] = '\\0';","            scratch[length] = '\\0';\n            text = comma + 1;").replace('            text = comma + 1;\n        }','        }'),
 'signed-length':vector.replace('unsigned length = comma - text;','int length = comma - text;'),
 'size-length':vector.replace('unsigned length = comma - text;','std::size_t length = comma - text;'),
 'scratch-pointer':vector.replace('std::memcpy(scratch, text, length);','std::memcpy(&scratch[0], text, length);'),
 'preincrement':vector.replace('for (int i = 0; i < 3; ++i)', 'for (int i = 0; i < 3; i++)'),
 'while-index':vector.replace('for (int i = 0; i < 3; ++i) {','int i=0; while (i < 3) {').replace('            text = comma + 1;','            text = comma + 1;\n            ++i;'),
 'do-while':vector.replace('for (int i = 0; i < 3; ++i) {','int i=0; do {').replace('            text = comma + 1;\n        }','            text = comma + 1;\n            ++i;\n        } while (i < 3);'),
}
flags={
 'r-j1':['-O2','-frename-registers','-falign-jumps=1'],
 'r-j4':['-O2','-frename-registers','-falign-jumps=4'],
 'r-j8':['-O2','-frename-registers','-falign-jumps=8'],
 'plain':['-O2'],
 'r-stack2':['-O2','-frename-registers','-falign-jumps=1','-mpreferred-stack-boundary=2'],
 'r-stack2-j4':['-O2','-frename-registers','-falign-jumps=4','-mpreferred-stack-boundary=2'],
}
T=Path('/tmp/vuc-binutils215/vuc-value-set-ast-abi-oct15b');T.mkdir(parents=True,exist_ok=True)
rank=[];sizes=collections.Counter();best=None
for (bk,bval),(lk,lval),(fk,opts) in itertools.product(boolforms.items(),loopforms.items(),flags.items()):
 tag=f'{bk}-{lk}-{fk}'
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o')
 cpp.write_text(base.replace(section,bval).replace(vector,lval))
 cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if cp.returncode:rank.append(dict(tag=tag,error=cp.stderr[-140:]));continue
 # Inspect authentic actual size even when disqualified; the strict
 # original relocation and literal checker only runs on equal length.
 try: result=m.run(obj,cfg)
 except ValueError as e:
  from subprocess import check_output
  sym=next((line for line in check_output(['nm','-S',str(obj)],text=True).splitlines() if line.endswith('_ZN5Value3SetEPK15ASTConstValExpr')),None)
  length=int(sym.split()[1],16) if sym else -1
  rank.append(dict(tag=tag,length=length,strict_error=str(e)[:125]));sizes[length]+=1
  continue
 row=dict(tag=tag,length=result['length'],match=result['exact_bytes'],relocations=result['relocations'],prefix=next(iter(result['different_offsets']),result['length']))
 rank.append(row);sizes[row['length']]+=1
 if (best is None or (row['match'],row['prefix'])>(best['match'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(cpp.read_text());(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(opts)+'\n')
(T/'results.json').write_text(json.dumps(rank,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rank),sizes=dict(sizes),best=best,exact_size=[x for x in rank if x.get('length')==455][:12]),indent=2))
