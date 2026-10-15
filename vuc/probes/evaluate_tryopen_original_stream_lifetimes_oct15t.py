#!/usr/bin/env python3
"""Reconstruct original TryOpenFile C++03 stream ownership, not C ABI stubs.

Original ELF has a real GNU iostream object, its virtual-subobject
init, basic_filebuf.open mode 8, a fail-state test and complete EH
cleanup. Vary real historical ifstream/fstream construction and
observer lifetime source without changing the fixed compiler snapshot.
"""
from pathlib import Path
import subprocess,json
R=Path(__file__).resolve().parents[1]
O=Path('/tmp/vuc-binutils215/tryopen-original-stream-lifetimes-oct15t')
O.mkdir(exist_ok=True)
forms={
 'ifstream-ctor-fail':'std::ifstream f(path); return !f.fail();',
 'ifstream-ctor-if-fail':'std::ifstream f(path); if (f.fail()) return false; return true;',
 'ifstream-ctor-if-stream':'std::ifstream f(path); if (f) return true; return false;',
 'ifstream-ctor-not-stream':'std::ifstream f(path); if (!f) return false; return true;',
 'ifstream-ctor-good':'std::ifstream f(path); return f.good();',
 'ifstream-ctor-rdstate':'std::ifstream f(path); return !(f.rdstate() & (std::ios::badbit|std::ios::failbit));',
 'ifstream-ctor-rdbuf':'std::ifstream f(path); return f.rdbuf()->is_open();',
 'ifstream-explicit-in':'std::ifstream f(path,std::ios::in); return !f.fail();',
 'ifstream-default-open':'std::ifstream f; f.open(path); return !f.fail();',
 'ifstream-default-open-if':'std::ifstream f; f.open(path); if(f.fail())return false;return true;',
 'fstream-ctor-in':'std::fstream f(path,std::ios::in); return !f.fail();',
 'fstream-ctor-if':'std::fstream f(path,std::ios::in); if(!f)return false;return true;',
 'fstream-default-open':'std::fstream f; f.open(path,std::ios::in); return !f.fail();',
 'fstream-default-open-if':'std::fstream f; f.open(path,std::ios::in); if(f.fail())return false;return true;',
 'fstream-default-rdbuf':'std::fstream f; f.open(path,std::ios::in); return f.rdbuf()->is_open();',
}
rows=[]
for tag,body in forms.items():
 cpp=O/(tag+'.cpp')
 cpp.write_text('#include <fstream>\nbool TryOpenFile(const char *path) {\n '+body+'\n}\n')
 for snap in ('jan25','dec06'):
  obj=O/(tag+'-'+snap+'.o')
  p=subprocess.run(['/tmp/vuc-binutils215/g++-'+snap+'-as215','-O2','-frename-registers','-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
  row={'source':tag,'compiler':snap}
  if p.returncode:row['error']=p.stderr[-170:]
  else:
   nm=subprocess.run(['nm','-S','--defined-only',str(obj)],capture_output=True,text=True)
   sy=[x.split() for x in nm.stdout.splitlines() if x.endswith('_Z11TryOpenFilePKc') and len(x.split())>=4]
   row['length']=int(sy[0][1],16) if sy else -1
  rows.append(row);print(row,flush=True)
(O/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
