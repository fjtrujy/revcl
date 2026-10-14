#!/usr/bin/env python3
"""C++03 Value::ToString real buffer and receiver ownership controls.

The original 221-byte routine has 8 authenticated relocations and 82
original literal bytes. Test plausible source-level buffer aliases, explicit
string return value ownership, and receiver lifetimes; never patch opcode
bytes, force registers or fake data addresses.
"""
from pathlib import Path
import importlib.util,itertools,subprocess,json,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=R/'vuc/config/value_to_string_oct13_probe.json'
base=(R/'vuc/probes/ValueToStringUnit.cpp').read_text().replace(
 '#include "../src/MLGameString.h"',
 '#include "'+str(R/'vuc/src/MLGameString.h')+'"')
start=base.index('MLGameString Value::ToString() const')
pre=base[:start];fun=base[start:]
flags={
 'baseline':['-O2','-frename-registers','-falign-jumps=4'],
 'j4l1':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=1'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
 'j1':['-O2','-frename-registers','-falign-jumps=1'],
 'rj4noreorder':['-O2','-frename-registers','-falign-jumps=4','-fno-reorder-blocks'],
 'rj4nosched':['-O2','-frename-registers','-falign-jumps=4','-fno-schedule-insns2'],
 'plain':['-O2','-falign-jumps=4'],
}
forms={
 'baseline':lambda s:s,
 'dest-pointer':lambda s:s.replace('    switch (type)', '    char *output = buffer;\n    switch (type)').replace('std::sprintf(buffer,','std::sprintf(output,').replace('MLGameString(buffer, false)','MLGameString(output, false)'),
 'dest-const':lambda s:s.replace('    switch (type)', '    char *const output = buffer;\n    switch (type)').replace('std::sprintf(buffer,','std::sprintf(output,').replace('MLGameString(buffer, false)','MLGameString(output, false)'),
 'dest-array-ref':lambda s:s.replace('    switch (type)', '    char (&output)[64] = buffer;\n    switch (type)').replace('std::sprintf(buffer,','std::sprintf(output,').replace('MLGameString(buffer, false)','MLGameString(output, false)'),
 'buffer-zero':lambda s:s.replace('char buffer[64] = "";','char buffer[64] = {0};'),
 'buffer-empty-nul':lambda s:s.replace('char buffer[64] = "";','char buffer[64] = "\\0";'),
 'return-local':lambda s:s.replace('    return MLGameString(buffer, false);','    MLGameString result(buffer, false);\n    return result;'),
 'return-const-ref':lambda s:s.replace('    return MLGameString(buffer, false);','    const MLGameString &result = MLGameString(buffer, false);\n    return result;'),
 'type-local':lambda s:s.replace('    switch (type)', '    int tag = type;\n    switch (tag)'),
 'type-unsigned':lambda s:s.replace('    switch (type)', '    unsigned tag = (unsigned)type;\n    switch (tag)'),
 'receiver-pointer':lambda s:s.replace('    switch (type)', '    const Value *receiver = this;\n    switch (receiver->type)').replace('"%d", bits','"%d", receiver->bits').replace('values[','receiver->values['),
 'receiver-ref':lambda s:s.replace('    switch (type)', '    const Value &receiver = *this;\n    switch (receiver.type)').replace('"%d", bits','"%d", receiver.bits').replace('values[','receiver.values['),
 'local-int-format':lambda s:s.replace('        std::sprintf(buffer, "%d", bits);','        const char *fmt = "%d";\n        std::sprintf(buffer, fmt, bits);'),
 'local-scalar-format':lambda s:s.replace('        std::sprintf(buffer, "%f", values[0]);','        const char *fmt = "%f";\n        std::sprintf(buffer, fmt, values[0]);'),
 'local-vector-format':lambda s:s.replace('        std::sprintf(buffer, "%f,%f,%f,%f",','        const char *fmt = "%f,%f,%f,%f";\n        std::sprintf(buffer, fmt,'),
}
T=Path('/tmp/vuc-binutils215/value-tostring-lifetime-oct15');T.mkdir(parents=True,exist_ok=True)
rows=[];sizes=collections.Counter();best=None
for (name,form),(mode,opts) in itertools.product(forms.items(),flags.items()):
 tag=f'{name}-{mode}'
 source=pre+form(fun)
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(source)
 p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:rows.append(dict(tag=tag,error=p.stderr[-130:]));continue
 try:r=m.run(obj,cfg)
 except Exception as e:rows.append(dict(tag=tag,error=str(e)));continue
 row=dict(tag=tag,length=r['length'],overlap=r['exact_bytes'],prefix=next(iter(r['different_offsets']),r['length']),relocations=r['relocations'],literal_bytes=r['verified_literal_bytes'])
 rows.append(row);sizes[row['length']]+=1
 if row['length']==221 and row['relocations']==8 and row['literal_bytes']==82 and (best is None or (row['overlap'],row['prefix'])>(best['overlap'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(source);(T/'best.flags').write_text(' '.join(opts)+'\n');(T/'best.o').write_bytes(obj.read_bytes())
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best=best,top=sorted((r for r in rows if 'overlap' in r),key=lambda x:(x['overlap'],x['prefix']),reverse=True)[:12]),indent=2))
