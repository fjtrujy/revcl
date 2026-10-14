#!/usr/bin/env python3
"""Try genuine C++ case order and format source spellings for Value::ToString.

Retain original 221-byte layout, all eight named relocations and all 82
literal bytes; retain real +,-,* preceding methods in the source unit.
"""
from pathlib import Path
import importlib.util,json,itertools,subprocess,collections
R=Path(__file__).resolve().parents[2];sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
base=(R/'vuc/probes/ValueToStringUnit.cpp').read_text().replace(
    '#include "../src/MLGameString.h"',
    '#include "'+str(R/'vuc/src/MLGameString.h')+'"')
start=base.index('    case 1:',base.index('MLGameString Value::ToString() const'))
stop=base.index('\n    }\n    return MLGameString(buffer, false);',start)
arms={
 'int':'case 1:\n    case 2:\n        std::sprintf(buffer, "%d", bits);\n        break;',
 'scalar':'case 3:\n        std::sprintf(buffer, "%f", values[0]);\n        break;',
 'vector':'case 4:\n        std::sprintf(buffer, "%f,%f,%f,%f",\n                     values[0], values[1], values[2], values[3]);\n        break;',
}
assert all(arm in base[start:stop] for arm in arms.values())
variants={'normal':base}
for order in itertools.permutations(arms):
 name='-'.join(order)
 variants[name]=base[:start]+'    '+'\n    '.join(arms[k] for k in order)+base[stop:]
 # Semantics: order of named switch cases unchanged, compiler ordering only
flags={
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j4-l1':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=1'],
 'j4-l4':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=4'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
 'j16':['-O2','-frename-registers','-falign-jumps=16'],
 'j4-sched':['-O2','-frename-registers','-falign-jumps=4','-fno-schedule-insns2'],
}
T=Path('/tmp/vuc-binutils215/value-tostring-real-switch-order-next');T.mkdir(parents=True,exist_ok=True)
records=[];best=None;sizes=collections.Counter()
for (name,source),(fl,opts) in itertools.product(variants.items(),flags.items()):
 tag=name+'-'+fl;cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(source)
 c=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if c.returncode:records.append(dict(tag=tag,error=c.stderr[-170:]));continue
 try:result=m.run(obj,R/'vuc/config/value_to_string_oct13_probe.json')
 except ValueError as e:records.append(dict(tag=tag,error=str(e)));continue
 row=dict(tag=tag,length=result['length'],overlap=result['exact_bytes'],relocations=result['relocations'],verified_literal_bytes=result['verified_literal_bytes'],prefix=next((i for i in range(221) if i in result['different_offsets']),221))
 records.append(row);sizes[row['length']]+=1
 if row['length']==221 and row['relocations']==8 and row['verified_literal_bytes']==82 and (best is None or row['overlap']>best['overlap']):
  best=row;(T/'best.cpp').write_text(source);(T/'best.flags').write_text(' '.join(opts)+'\n');(T/'best.o').write_bytes(obj.read_bytes())
(T/'results.json').write_text(json.dumps(records,indent=2)+'\n')
print(json.dumps(dict(attempted=len(records),sizes=dict(sizes),best_original_length=best,top=sorted((r for r in records if 'overlap'in r),key=lambda r:r['overlap'],reverse=True)[:10]),indent=2))
