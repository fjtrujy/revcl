#!/usr/bin/env python3
"""Compiler-driven controls for actual VUC AsmOutput_PrintInstruction directive scan.

Original ELF initializes BL=0 before the .assign strstr call, sets BL=1
only on a nonzero result, and probes .equ only if .assign was absent.
Test ordinary 2003 C++ bool/int flags with identical observable behavior.
"""
from pathlib import Path
import json,subprocess
R=Path(__file__).resolve().parents[1]
O=Path('/tmp/vuc-binutils215/asm-output-directive-bool-oct15r')
O.mkdir(exist_ok=True)
source=(R/'probes/AsmOutputPrintInstructionFreshOct15r.cpp').read_text()
source=source.replace('#include "../src/MLGameString.h"','#include "'+str(R/'src/MLGameString.h')+'"')
start=source.index('    bool isConstant=')
stop=source.index('    if (hasEndMacro)',start)
var={
 'initial':source[start:stop],
 'bool-if':'''    bool isConstant=false;
    if (std::strstr(text,".assign")) isConstant=true;
    if (!isConstant) isConstant=std::strstr(text,".equ")!=0;
''',
 'int-if':'''    int isConstant=0;
    if (std::strstr(text,".assign")) isConstant=1;
    if (!isConstant) isConstant=std::strstr(text,".equ")!=0;
''',
 'char-if':'''    char isConstant=0;
    if (std::strstr(text,".assign")) isConstant=1;
    if (!isConstant) isConstant=std::strstr(text,".equ")!=0;
''',
 'unsigned-if':'''    unsigned isConstant=0;
    if (std::strstr(text,".assign")) isConstant=1;
    if (!isConstant) isConstant=std::strstr(text,".equ")!=0;
''',
 'bool-lazy':'''    bool isConstant=false;
    if (std::strstr(text,".assign")) isConstant=true;
    else if (std::strstr(text,".equ")) isConstant=true;
''',
 'int-lazy':'''    int isConstant=0;
    if (std::strstr(text,".assign")) isConstant=1;
    else if (std::strstr(text,".equ")) isConstant=1;
''',
 'bool-goto':'''    bool isConstant=false;
    if (std::strstr(text,".assign")) { isConstant=true; goto have_directive; }
    if (std::strstr(text,".equ")) isConstant=true;
    have_directive:;
''',
 'int-goto':'''    int isConstant=0;
    if (std::strstr(text,".assign")) { isConstant=1; goto have_directive; }
    if (std::strstr(text,".equ")) isConstant=1;
    have_directive:;
''',
}
rows=[]
for tag,part in var.items():
 cpp=O/(tag+'.cpp');cpp.write_text(source[:start]+part+source[stop:])
 for snap in ('jan25','dec06'):
  obj=O/(tag+'-'+snap+'.o')
  proc=subprocess.run(['/tmp/vuc-binutils215/g++-'+snap+'-as215','-O2','-frename-registers','-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
  row={'variant':tag,'snapshot':snap}
  if proc.returncode:row['error']=proc.stderr[-200:]
  else:
   nm=subprocess.run(['nm','-S','--defined-only',str(obj)],capture_output=True,text=True)
   sym=[x.split() for x in nm.stdout.splitlines() if len(x.split())>=4 and x.split()[3]=='_Z26AsmOutput_PrintInstructionP11InstructionRSt14basic_ofstreamIcSt11char_traitsIcEE']
   row['length']=int(sym[0][1],16) if sym else None
  rows.append(row)
  print(row,flush=True)
(O/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
