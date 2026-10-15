#!/usr/bin/env python3
"""Test original IR Type::Translate contiguous 1/2/3 switch ownership.

Original ELF repeats compare kind 2, branches >2 to kind 3 and decrements
for kind 1, consistent with historical GCC switch lowering. Keep actual
named IR polymorphic type constructors and test source-level fallthrough.
"""
from pathlib import Path
import subprocess,json
R=Path(__file__).resolve().parents[2]/'vuc'
O=Path('/tmp/vuc-binutils215/translate-type-source-dispatch-oct15q')
O.mkdir(exist_ok=True)
original=(R/'probes/TranslateTypeFullOct15q.cpp').read_text()
start=original.index('    if (kind == 2)')
end=original.index('    if ((type.flags & 0x100)',start)
variants={
 'if-chain': original[start:end],
 'switch-return': '''    switch (kind) {
        case 1:
          if (!((const unsigned char *)&type)[15])
              return new IR::Int_Data;
          return new IR::UInt_Data;
        case 2:
          return new IR::UInt_Data;
        case 3:
          return new IR::Float_Data(((int)type.width)>>10);
    }
''',
 'switch-fallthrough': '''    switch (kind) {
        case 1:
          if (!((const unsigned char *)&type)[15])
              return new IR::Int_Data;
        case 2:
          return new IR::UInt_Data;
        case 3:
          return new IR::Float_Data(((int)type.width)>>10);
    }
''',
 'switch-default-mapped': '''    switch (kind) {
        case 1:
          if (((const unsigned char *)&type)[15])
              return new IR::UInt_Data;
          return new IR::Int_Data;
        case 2:
          return new IR::UInt_Data;
        case 3:
          return new IR::Float_Data(((int)type.width)>>10);
        default: break;
    }
''',
 'if-first-one': '''    if (kind == 1 && !((const unsigned char *)&type)[15])
        return new IR::Int_Data;
    if (kind == 2 || kind == 1)
        return new IR::UInt_Data;
    if (kind == 3)
        return new IR::Float_Data(((int)type.width)>>10);
''',
}
rows=[]
for name,dispatch in variants.items():
 cpp=O/(name+'.cpp');cpp.write_text(original[:start]+dispatch+original[end:])
 for tag,flags in {
   'o2':['-O2','-frename-registers'],
   'j1':['-O2','-frename-registers','-falign-jumps=1'],
   'j4':['-O2','-frename-registers','-falign-jumps=4'],
   'j8':['-O2','-frename-registers','-falign-jumps=8'],
 }.items():
  obj=O/(name+'-'+tag+'.o')
  p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
  row={'case':name,'compiler_flags':tag}
  if p.returncode:row['error']=p.stderr[-200:]
  else:
   nm=subprocess.run(['nm','-S','--defined-only',str(obj)],capture_output=True,text=True)
   syms=[x.split() for x in nm.stdout.splitlines() if len(x.split())>=4 and x.split()[3]=='_Z9TranslateRK4Typeji']
   row['size']=int(syms[0][1],16) if syms else -1
  rows.append(row)
  print(row,flush=True)
(O/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
