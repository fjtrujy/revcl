#!/usr/bin/env python3
"""Preserve the real Value::Set dispatch while matching constructor unit startup."""
import importlib.util
from pathlib import Path
import subprocess
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/value-set-context-oct13');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
source=(R/'vuc/probes/value_mul_unit_ops_add_mul.cpp').read_text()
source=source.replace('Value() : type(0),a(0),b(0),flag0(0),flag1(0),flag2(0),flag3(0) {}','Value();')
source=source.replace('    Value operator*(const Value &other) const;','    Value operator*(const Value &other) const;\n    void Set(const ASTExpr *expr);\n    void Set(const ASTLValExpr *expr);\n    void Set(const ASTConstValExpr *expr);')
header='''#include <iostream>
struct ASTExpr { unsigned field0, field1; unsigned kind; };
struct ASTLValExpr;
struct ASTConstValExpr;
'''+source[:source.index('Value Value::operator==')]
header+='Value::Value() : type(0),a(0),b(0),flag0(0),flag1(0),flag2(0),flag3(0) {}\n'
set_body='''void Value::Set(const ASTExpr *expr)
{
    if (expr->kind == 2)
        Set((const ASTConstValExpr *)expr);
    else if (expr->kind == 3)
        Set((const ASTLValExpr *)expr);
}
'''
compare=source[source.index('Value Value::operator=='):source.index('Value Value::operator+(')]
plus=source[source.index('Value Value::operator+('):source.index('Value Value::operator*(')]
mul=source[source.index('Value Value::operator*('):]
variants={
 'set-before-all':[set_body,plus,mul,compare],
 'set-after-all':[plus,mul,compare,set_body],
 'set-after-plus':[plus,set_body,mul,compare],
 'set-after-mul':[plus,mul,set_body,compare],
 'set-after-compare':[compare,set_body,plus,mul],
}
flags=['-O2','-frename-registers','-fuse-cxa-atexit','-ffunction-sections']
probes=['iostream_value_wrapper_oct12_probe','value_default_c1_probe','value_default_c2_probe','value_dispatch_exact_probe']
for key,parts in variants.items():
 path=T/(key+'.cpp');path.write_text(header+'\n'.join(parts))
 obj=T/(key+'.o')
 cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(path),'-o',str(obj)],capture_output=True,text=True)
 if cp.returncode: print(key,'compile error',cp.stderr[-300:],flush=True);continue
 vals=[]
 for p in probes:
  try:
   r=m.run(obj,R/'vuc/config'/(p+'.json'))
   vals.append(p+': '+str(r['exact_bytes'])+'/'+str(r['length']))
   if p=='iostream_value_wrapper_oct12_probe' and r['exact']:
    print('EXACT STARTUP',key,flush=True)
  except ValueError as e:vals.append(p+': '+str(e)[:65])
 print(key,'; '.join(vals),flush=True)
