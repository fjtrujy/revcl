#!/usr/bin/env python3
"""Recover VUC Value::ToString's original 221-byte control-flow structure.

Probe genuine source-level switch alternatives, preserving verified 82-byte
literal data and all eight authentic sprintf / string constructor relocations.
No artificial padding, machine code, or forced-register assembly.
"""
from pathlib import Path
import importlib.util,itertools,subprocess,json,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=R/'vuc/config/value_to_string_oct13_probe.json'
base=(R/'vuc/probes/ValueToStringUnit.cpp').read_text().replace('#include "../src/MLGameString.h"','#include "'+str(R/'vuc/src/MLGameString.h')+'"')
start=base.index('    switch (type) {',base.index('MLGameString Value::ToString() const'))
stop=base.index('    return MLGameString(buffer, false);',start)
arms={
 "integer":('type==1 || type==2','std::sprintf(buffer, "%d", bits);'),
 "scalar":('type==3','std::sprintf(buffer, "%f", values[0]);'),
 "vector":('type==4','std::sprintf(buffer, "%f,%f,%f,%f", values[0], values[1], values[2], values[3]);')
}
patterns={
 "else-if": lambda names:'    '+' else '.join('if ('+arms[n][0]+') { '+arms[n][1]+' }' for n in names)+'\n',
 "independent-if":lambda names:'    '+'\n    '.join('if ('+arms[n][0]+') { '+arms[n][1]+' }' for n in names)+'\n',
 "switch":lambda names:'    switch (type) {\n'+'\n'.join(('    case 1: case 2: ' if n=="integer" else '    case 3: ' if n=="scalar" else '    case 4: ')+arms[n][1]+' break;' for n in names)+'\n    }\n',
 "guard-else":lambda names:'    '+'\n    else '.join('if (!('+arms[n][0]+')) {} else { '+arms[n][1]+' }' for n in names)+'\n',
}
flags={
 "j1":["-O2","-frename-registers","-falign-jumps=1"],
 "j4":["-O2","-frename-registers","-falign-jumps=4"],
 "j8":["-O2","-frename-registers","-falign-jumps=8"],
 "j16":["-O2","-frename-registers","-falign-jumps=16"],
 "j4l1":["-O2","-frename-registers","-falign-jumps=4","-falign-labels=1"],
 "j4l8":["-O2","-frename-registers","-falign-jumps=4","-falign-labels=8"],
 "j8l1":["-O2","-frename-registers","-falign-jumps=8","-falign-labels=1"],
 "no-reorder":["-O2","-frename-registers","-falign-jumps=4","-fno-reorder-blocks"],
}
T=Path('/tmp/vuc-binutils215/value-tostring-source-dispatch-oct15');T.mkdir(parents=True,exist_ok=True)
records=[];lengths=collections.Counter();best=None
for mode,order,fl in itertools.product(patterns,itertools.permutations(arms),flags):
 key=f'{mode}-'+'-'.join(order)+'-'+fl
 text=base[:start]+patterns[mode](order)+base[stop:]
 cpp=T/(key+'.cpp');obj=T/(key+'.o');cpp.write_text(text)
 p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags[fl],'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:records.append(dict(key=key,error=p.stderr[-190:]));continue
 try:result=m.run(obj,cfg)
 except Exception as e:records.append(dict(key=key,error=str(e)));continue
 prefix=next((i for i in range(result['length']) if i in result['different_offsets']),result['length'])
 row=dict(key=key,length=result['length'],overlap=result['exact_bytes'],prefix=prefix,relocations=result['relocations'],literal_bytes=result['verified_literal_bytes'])
 records.append(row);lengths[row['length']]+=1
 if row['length']==221 and row['relocations']==8 and row['literal_bytes']==82 and (best is None or (row['overlap'],row['prefix'])>(best['overlap'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(text);(T/'best.flags').write_text(' '.join(flags[fl])+'\n');(T/'best.o').write_bytes(obj.read_bytes())
(T/'results.json').write_text(json.dumps(records,indent=2)+'\n')
print(json.dumps(dict(attempted=len(records),sizes=dict(lengths),best_original_length=best,top=sorted((x for x in records if 'overlap'in x),key=lambda r:(r['overlap'],r['prefix']),reverse=True)[:10]),indent=2))
