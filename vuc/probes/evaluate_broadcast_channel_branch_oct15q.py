#!/usr/bin/env python3
"""Original 2004 C++03 BroadcastMove branch and return-copy source controls.

ELF proves initial test [register+4] &7, then &8 before copying return;
only the mask branch creates four MLTempString temporary source fragments.
Preserve the full channel masks and authentic instruction format literals.
"""
from pathlib import Path
import subprocess,json
R=Path(__file__).resolve().parents[2]/'vuc'
O=Path('/tmp/vuc-binutils215/broadcast-channel-flow-oct15q')
O.mkdir(exist_ok=True)
base=(R/'probes/BroadcastMoveFullReconstructionOct15q.cpp').read_text().replace('#include "../src/MLGameString.h"','#include "'+str(R/'src/MLGameString.h')+'"')
header='''    unsigned mask = result.channels & 7;
    if (mask == 0 && (result.channels & 8)) {
        output.Add("mul.w %s, vf00, %s", result.name.c_str(), input.name.c_str());
    } else if (mask) {'''
assert header in base
body=base[base.index('        MLTempString channels(".");'):base.index('    }\n    return result;')]
variants={
 'baseline':base,
 'branch-test-no-initial-mask':base.replace(header,'''    if (!(result.channels & 7)) {
        if (result.channels & 8)
            output.Add("mul.w %s, vf00, %s", result.name.c_str(), input.name.c_str());
    } else {
        unsigned mask = result.channels & 7;'''),
 'branch-test-mask-in-else':base.replace(header,'''    if (result.channels & 7) {
        unsigned mask = result.channels & 7;''').replace('    }\n    return result;','''    } else if (result.channels & 8) {
        output.Add("mul.w %s, vf00, %s", result.name.c_str(), input.name.c_str());
    }
    return result;'''),
 'inverted-double-test':base.replace(header,'''    if ((result.channels & 7) == 0) {
        if (result.channels & 8)
            output.Add("mul.w %s, vf00, %s", result.name.c_str(), input.name.c_str());
    } else {
        unsigned mask = result.channels & 7;'''),
 'local-full-channel-flags':base.replace(header,'''    unsigned flags = result.channels;
    if ((flags & 7) == 0) {
        if (flags & 8)
            output.Add("mul.w %s, vf00, %s", result.name.c_str(), input.name.c_str());
    } else {
        unsigned mask = flags & 7;'''),
 'early-return':base.replace(header,'''    if ((result.channels & 7) == 0) {
        if ((result.channels & 8) == 0)
            return result;
        output.Add("mul.w %s, vf00, %s", result.name.c_str(), input.name.c_str());
    } else {
        unsigned mask = result.channels & 7;'''),
}
rows=[]
for name,src in variants.items():
 cpp=O/(name+'.cpp');cpp.write_text(src)
 for flags,opts in {
   'o2':['-O2'],
   'o2r':['-O2','-frename-registers'],
   'j1':['-O2','-frename-registers','-falign-jumps=1'],
   'j4':['-O2','-frename-registers','-falign-jumps=4'],
   'j8':['-O2','-frename-registers','-falign-jumps=8'],
   'j16':['-O2','-frename-registers','-falign-jumps=16'],
 }.items():
  obj=O/(name+'-'+flags+'.o')
  p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
  row={'source':name,'flags':flags}
  if p.returncode:row['error']=p.stderr[-190:]
  else:
   nm=subprocess.run(['nm','-S','--defined-only',str(obj)],capture_output=True,text=True)
   syms=[x.split() for x in nm.stdout.splitlines() if len(x.split())>=4 and x.split()[3]=='_Z13BroadcastMoveRKN2IR8RegisterES2_R15InstructionList']
   row['size']=int(syms[0][1],16) if syms else -1
  rows.append(row)
  print(name,flags,row.get('size'),row.get('error',''),flush=True)
(O/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
