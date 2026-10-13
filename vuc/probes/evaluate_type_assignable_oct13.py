#!/usr/bin/env python3
"""Preserve semantics while recovering the original Type::IsAssignable GCC 3.3 CFG."""
import importlib.util
import subprocess
from pathlib import Path
R = Path(__file__).resolve().parents[2]
P = Path('/tmp/vuc-binutils215/type-assign-oct13')
P.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location('matcher', R/'vuc/scripts/match_probe.py')
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
original = (R/'vuc/probes/type_is_assignable.cpp').read_text()
head = original[:original.index('bool Type::IsAssignable(')]
variants={
'early':'''bool Type::IsAssignable(const Type &other,bool allow) const {
 if (flags != other.flags) {
  if ((flags & 0xffffc3ffu) != 4) return false;
  if ((other.flags & 0xffffc3ffu) != 4) return false;
  if ((other.flags & flags) != flags) return false;
 } else {
  if ((flags & 0x100u) && !pointer && other.pointer) return false;
  if (flag && (!other.flag || !allow)) return false;
 }
 return true;
}''',
'early-a':'''bool Type::IsAssignable(const Type &other,bool allow) const {
 if (flags == other.flags) {
  if ((flags & 0x100u) && !pointer && other.pointer) return false;
  if (flag && (!other.flag || !allow)) return false;
 } else {
  if ((flags & 0xffffc3ffu) != 4) return false;
  if ((other.flags & 0xffffc3ffu) != 4) return false;
  if ((other.flags & flags) != flags) return false;
 }
 return true;
}''',
'guard':'''bool Type::IsAssignable(const Type &other,bool allow) const {
 if (flags != other.flags) {
  if ((flags & 0xffffc3ffu) != 4 || (other.flags & 0xffffc3ffu) != 4 ||
     (other.flags & flags) != flags) return false;
 } else {
  if ((flags & 0x100u) && !pointer && other.pointer) return false;
  if (flag && (!other.flag || !allow)) return false;
 }
 return true;
}''',
'guard-noelse':'''bool Type::IsAssignable(const Type &other,bool allow) const {
 if (flags != other.flags) {
  if ((flags & 0xffffc3ffu) != 4) return false;
  if ((other.flags & 0xffffc3ffu) != 4) return false;
  if ((other.flags & flags) != flags) return false;
  return true;
 }
 if ((flags & 0x100u) && !pointer && other.pointer) return false;
 if (flag && (!other.flag || !allow)) return false;
 return true;
}''',
'nested':'''bool Type::IsAssignable(const Type &other,bool allow) const {
 if (flags != other.flags) {
  if ((flags & 0xffffc3ffu) == 4 && (other.flags & 0xffffc3ffu) == 4) {
   if ((other.flags & flags) == flags) return true;
  }
  return false;
 }
 if ((flags & 0x100u) && !pointer && other.pointer) return false;
 if (flag && (!other.flag || !allow)) return false;
 return true;
}''',
'original':original[original.index('bool Type::IsAssignable('):],
}
flsets={'rename':['-O2','-frename-registers'], 'plain':['-O2'], 'j1':['-O2','-frename-registers','-falign-jumps=1'], 'j4':['-O2','-frename-registers','-falign-jumps=4'], 'j8':['-O2','-frename-registers','-falign-jumps=8']}
for label,body in variants.items():
 src=P/(label+'.cpp');src.write_text(head+body+'\n')
 for flagname, flags in flsets.items():
  obj=P/(label+'-'+flagname+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(src),'-o',str(obj)],capture_output=True,text=True)
  if cp.returncode: print('FAIL',label,flagname,cp.stderr[-140:],flush=True);continue
  symbol_line=next((x for x in subprocess.run(['nm','-S',str(obj)],capture_output=True,text=True).stdout.splitlines() if '_ZNK4Type12IsAssignable' in x),None)
  size=int(symbol_line.split()[1],16) if symbol_line else 0
  if size==151:
   try:
    result=matcher.run(obj,R/'vuc/config/type_is_assignable_probe.json')
    print(label,flagname,f'{result["exact_bytes"]}/151',flush=True)
    if result['exact']: print('EXACT',src,flags,flush=True);raise SystemExit(0)
   except ValueError as exc:print(label,flagname,str(exc)[:125],flush=True)
  else: print(label,flagname,'size',size,flush=True)
