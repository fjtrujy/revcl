#!/usr/bin/env python3
"""Preserve unusual original C++03 masked-channel fallback and call order.

Original ELF tests (channels & 7) == 15, despite the impossible compare,
then zero, before either emitting an empty-format move or composing the
x/y/z suffix. This odd check might reflect actual human-written code.
Test source-aware masks and the c_str() call lifetime, not broad flags.
"""
from pathlib import Path
import subprocess,json
R=Path(__file__).resolve().parents[2]/'vuc'
O=Path('/tmp/vuc-binutils215/broadcast-authored-mask-oct15q')
O.mkdir(exist_ok=True)
base=(R/'probes/BroadcastMoveFullReconstructionOct15q.cpp').read_text()
base=base.replace('#include "../src/MLGameString.h"','#include "'+str(R/'src/MLGameString.h')+'"')
start=base.index('    } else if (mask) {')
end=base.index('\n    return result;',start)
old=base[start:end]
assert 'MLTempString fourth("");' in old
suffix=old[old.index('        MLTempString channels'):old.index('        output.Add("add%s %s, vf00, %s", channels.text')]
body='''    } else if (mask) {
        %s
        unsigned submask = result.channels & 7;
        if (submask == 15 || submask == 0) {
            output.Add("add%%s %%s, vf00, %%s", "", %s, %s);
        } else {
%s            output.Add("add%%s %%s, vf00, %%s", channels.text, %s, %s);
        }
    }'''
variants={}
for pre in ('no-pre','pre-before-mask','pre-after-mask'):
 for local in ('repeat-mask','direct-mask'):
  for fallback in ('normal','negated'):
   if pre=='no-pre':
    pp=''
    dst='result.name.c_str()';src='input.name.c_str()'
   else:
    pp='const char *inputName = input.name.c_str(); const char *resultName = result.name.c_str();'
    dst='resultName';src='inputName'
   maskstmt='unsigned submask = result.channels & 7;' if local=='repeat-mask' else 'unsigned submask = mask;'
   b=body%(pp, dst,src,suffix,dst,src)
   if pre=='pre-after-mask':
    b=b.replace(pp,'').replace(maskstmt,maskstmt+'\n '+pp)
   b=b.replace('unsigned submask = result.channels & 7;',maskstmt)
   if fallback=='negated':
    b=b.replace('submask == 15 || submask == 0','!(submask != 15 && submask != 0)')
   name='-'.join((pre,local,fallback))
   variants[name]=base[:start]+b+base[end:]
rows=[]
for name,src in variants.items():
 cpp=O/(name+'.cpp');cpp.write_text(src)
 for flags,opts in {'o2':['-O2','-frename-registers'],'j8':['-O2','-frename-registers','-falign-jumps=8']}.items():
  obj=O/(name+'-'+flags+'.o')
  p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
  row={'source':name,'flags':flags}
  if p.returncode:row['error']=p.stderr[-220:]
  else:
   nm=subprocess.run(['nm','-S','--defined-only',str(obj)],capture_output=True,text=True)
   syms=[x.split() for x in nm.stdout.splitlines() if len(x.split())>=4 and x.split()[3]=='_Z13BroadcastMoveRKN2IR8RegisterES2_R15InstructionList']
   row['size']=int(syms[0][1],16) if syms else -1
  rows.append(row)
  print(name,flags,row.get('size'),row.get('error',''),flush=True)
(O/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
