#!/usr/bin/env python3
"""Test original-era IR base/derived ctor data ownership in Canonize_FoldExpr.

Genuine ELF evidence: new 12/16 byte polymorphic IR constant objects
store kind at +4 before derived vtable pointer at +0. Test ordinary C++03
source constructors and object lifetime; no handwritten layout patches,
no inline assembly, and preserve exact acceptance gates.
"""
from pathlib import Path
import subprocess
import json

ROOT=Path(__file__).resolve().parents[2]
R=ROOT/'vuc'
OUT=Path('/tmp/vuc-binutils215/canonize-fold-class-constructors-oct15q')
OUT.mkdir(exist_ok=True)
original=(R/'probes/CanonizeFoldExprNewOct15p.cpp').read_text()
for inc in ('MLGameString.h',):
 original=original.replace('#include "../src/'+inc+'"','#include "'+str(R/'src'/inc)+'"')
variants={
 'baseline': original,
 'base-kind-init': original.replace('    unsigned kind;\n};', '    unsigned kind;\n    explicit Expr_Data(unsigned k):kind(k) {}\n};').replace('explicit ConstLit_Data(const MLGameString &v):name(v) {kind=7;}','explicit ConstLit_Data(const MLGameString &v):Expr_Data(7),name(v) {}').replace('ConstExpr_Data(Operator_Data *o,Expr_Data *e):op(o),expr(e) {kind=6;}','ConstExpr_Data(Operator_Data *o,Expr_Data *e):Expr_Data(6),op(o),expr(e) {}'),
 'base-kind-body': original.replace('    unsigned kind;\n};', '    unsigned kind;\n    explicit Expr_Data(unsigned k) { kind=k; }\n};').replace('explicit ConstLit_Data(const MLGameString &v):name(v) {kind=7;}','explicit ConstLit_Data(const MLGameString &v):Expr_Data(7),name(v) {}').replace('ConstExpr_Data(Operator_Data *o,Expr_Data *e):op(o),expr(e) {kind=6;}','ConstExpr_Data(Operator_Data *o,Expr_Data *e):Expr_Data(6),op(o),expr(e) {}'),
 'base-ctor-derived-body': original.replace('    unsigned kind;\n};', '    unsigned kind;\n    explicit Expr_Data(unsigned k):kind(k) {}\n};').replace('explicit ConstLit_Data(const MLGameString &v):name(v) {kind=7;}','explicit ConstLit_Data(const MLGameString &v):Expr_Data(7) {name=v;}').replace('ConstExpr_Data(Operator_Data *o,Expr_Data *e):op(o),expr(e) {kind=6;}','ConstExpr_Data(Operator_Data *o,Expr_Data *e):Expr_Data(6) {op=o;expr=e;}'),
 'derived-copy-body': original.replace('explicit ConstLit_Data(const MLGameString &v):name(v) {kind=7;}','explicit ConstLit_Data(const MLGameString &v):name(v) {kind=7;}').replace('ConstExpr_Data(Operator_Data *o,Expr_Data *e):op(o),expr(e) {kind=6;}','ConstExpr_Data(Operator_Data *o,Expr_Data *e):op(o),expr(e) {kind=6;}'),
 'base-kind-late-members': original.replace('    unsigned kind;\n};', '    unsigned kind;\n    explicit Expr_Data(unsigned k):kind(k) {}\n};').replace('explicit ConstLit_Data(const MLGameString &v):name(v) {kind=7;}','explicit ConstLit_Data(const MLGameString &v):Expr_Data(7),name(v) {}').replace('ConstExpr_Data(Operator_Data *o,Expr_Data *e):op(o),expr(e) {kind=6;}','ConstExpr_Data(Operator_Data *o,Expr_Data *e):Expr_Data(6) {op=o;expr=e;}'),
}
rows=[]
for tag,src in variants.items():
 src=src.replace('    unsigned kind;\n    explicit Expr_Data(unsigned k)',
                 '    unsigned kind;\n    Expr_Data():kind(0) {}\n    explicit Expr_Data(unsigned k)')
 file=OUT/(tag+'.cpp');file.write_text(src)
 for snapshot in ('jan25','dec06'):
  obj=OUT/(tag+'-'+snapshot+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-'+snapshot+'-as215','-O2','-frename-registers','-c',str(file),'-o',str(obj)],capture_output=True,text=True)
  row={'tag':tag,'snapshot':snapshot}
  if cp.returncode:row['error']=cp.stderr[-240:]
  else:
   proc=subprocess.run(['nm','-S','--defined-only',str(obj)],capture_output=True,text=True)
   lines=[r.split() for r in proc.stdout.splitlines() if len(r.split())>=4 and r.split()[3]=='_Z17Canonize_FoldExprPN2IR9Expr_DataE']
   if lines: row['length']=int(lines[0][1],16)
   proc=subprocess.run(['objdump','-dr','-Mintel','--disassemble=_Z17Canonize_FoldExprPN2IR9Expr_DataE',str(obj)],capture_output=True,text=True)
   row['first_allocation_instructions']=proc.stdout.splitlines()[105:150] if tag in ('baseline','base-kind-init') and snapshot=='jan25' else []
  rows.append(row)
  print(tag,snapshot,row.get('length'),row.get('error',''),flush=True)
(OUT/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
