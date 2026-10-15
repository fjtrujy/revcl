#!/usr/bin/env python3
"""Source-level VUC ParseArguments filename/path copy controls.

Original 777-byte ELF has rep movsl for the directory prefix and
built-in four-byte '.vcl' assignment. Test historically plausible
library idioms and C++03 loop ownership, recording original lengths
before ever considering strict relocation/literal matching.
"""
from pathlib import Path
import subprocess,json
R=Path(__file__).resolve().parents[1]
OUT=Path('/tmp/vuc-binutils215/parsearguments-copy-idioms-oct15t')
OUT.mkdir(exist_ok=True)
source=(R/'probes/ParseArgumentsFreshOct15t.cpp').read_text()
a='std::memcpy(inputPath,inputFileName,size);'
b='std::strcpy(outputFileName+std::strlen(outputFileName),".vcl");'
assert a in source and b in source
forms={
 'reference':(a,b),
 'strcat':(a,'std::strcat(outputFileName,".vcl");'),
 'builtin-memcpy':('__builtin_memcpy(inputPath,inputFileName,size);',b),
 'strncpy':('std::strncpy(inputPath,inputFileName,size);',b),
 'std-copy':('std::copy(inputFileName,slash+1,inputPath);',b),
 'manual-loop':('for (unsigned j=0;j<size;++j) inputPath[j]=inputFileName[j];',b),
 'manual-pointer':('const char *s=inputFileName;char *d=inputPath;while(s<=slash) *d++=*s++;',b),
 'two-byte-copy':('unsigned j=0;while(j<size) {inputPath[j]=inputFileName[j];++j;}',b),
 'prefix-assign':('std::copy(inputFileName,slash+1,inputPath);','std::strcat(outputFileName,".vcl");'),
}
results=[]
for label,(copy,append) in forms.items():
 cpp=OUT/(label+'.cpp')
 text=source.replace(a,copy).replace(b,append)
 if 'std::copy(' in text:text=text.replace('#include <cstring>','#include <cstring>\n#include <algorithm>')
 cpp.write_text(text)
 for compiler in ('jan25','dec06'):
  obj=OUT/(label+'-'+compiler+'.o')
  p=subprocess.run(['/tmp/vuc-binutils215/g++-'+compiler+'-as215','-O2','-frename-registers','-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
  row={'variant':label,'compiler':compiler}
  if p.returncode:row['error']=p.stderr[-130:]
  else:
   nm=subprocess.run(['nm','-S','--defined-only',str(obj)],capture_output=True,text=True)
   sym=[s.split() for s in nm.stdout.splitlines() if len(s.split())>=4 and s.split()[3]=='_Z14ParseArgumentsiPPc']
   row['length']=int(sym[0][1],16) if sym else None
   rel=subprocess.run(['readelf','-Wr',str(obj)],capture_output=True,text=True)
   row['path_memcpy_relocation']=' memcpy\n' in rel.stdout or '\tmemcpy\n' in rel.stdout
  results.append(row);print(row,flush=True)
(OUT/'results.json').write_text(json.dumps(results,indent=2)+'\n')
