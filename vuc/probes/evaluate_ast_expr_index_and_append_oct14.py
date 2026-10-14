#!/usr/bin/env python3
"""Second bounded 256-byte ASTExpr reconstruction batch, starting at 112/256.

Tests source-local index lives, actual append ordering and old array size,
and authentic historic source-unit compiler settings. Never patches emitted
object bytes or fabricates relocation targets.
"""
from pathlib import Path
import importlib.util,subprocess,itertools,collections,os
ROOT=Path(__file__).resolve().parents[2]
TMP=Path('/tmp/vuc-binutils215/ast-expr-better-index-oct14');TMP.mkdir(exist_ok=True)
sp=importlib.util.spec_from_file_location('m',ROOT/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
base=(Path('/tmp/vuc-binutils215/ast-expr-copy-lifetime-oct14')/'copy-loop-local-length--postinc-explicit.cpp').read_text()
variants={}
old='''    int i=0;
    while(i<input.length){
      int index=i; ++i;
      result.Add(Translate(input.entries[index],0x3c00,0));
    }'''
assert old in base
loops={
'original':old,
'index-postinc':old.replace('int index=i; ++i;','int index=i++;'),
'index-after':old.replace('int index=i; ++i;','int index=i;').replace('result.Add(Translate(input.entries[index],0x3c00,0));','result.Add(Translate(input.entries[index],0x3c00,0)); ++i;'),
'index-type-unsigned':old.replace('int i=0;','unsigned i=0;').replace('int index=i;','unsigned index=i;'),
'index-register':old.replace('int i=0;','register int i=0;').replace('int index=i;','register int index=i;'),
'index-vol':old.replace('int i=0;','volatile int i=0;'),
'index-result-local':old.replace('result.Add(Translate(input.entries[index],0x3c00,0));','IR::Expr_Data *translated=Translate(input.entries[index],0x3c00,0); result.Add(translated);'),
'index-after-resultlocal':old.replace('int index=i; ++i;','int index=i;').replace('result.Add(Translate(input.entries[index],0x3c00,0));','IR::Expr_Data *translated=Translate(input.entries[index],0x3c00,0); result.Add(translated); ++i;'),
'index-for':'    for(int i=0;i<input.length;++i) { int index=i; result.Add(Translate(input.entries[index],0x3c00,0)); }',
'index-for-volatile':'    for(volatile int i=0;i<input.length;++i) { int index=i; result.Add(Translate(input.entries[index],0x3c00,0)); }',
'index-continuous':'    int i=0; for(;;) { if(i>=input.length)break; int index=i++; result.Add(Translate(input.entries[index],0x3c00,0)); }',
'index-branch':'    int i=0; if(i<input.length) do {int index=i; ++i; result.Add(Translate(input.entries[index],0x3c00,0));} while(i<input.length);',
'index-prefix':'    int i=0; while(i<input.length) {int index=++i-1; result.Add(Translate(input.entries[index],0x3c00,0));}',
}
addvars={
'basic': lambda s:s,
'copy-oldsize':lambda s:s.replace('for (int j=0; j<length; ++j)','for (int j=0; j<old_size; ++j)'),
'newcap-const':lambda s:s.replace('int new_capacity = capacity ? 2 * capacity : 16;', 'const int new_capacity = capacity ? capacity*2 : 16;'),
'copy-local-ptr':lambda s:s.replace('if (old_entries)\n                for (int j=0; j<length; ++j) entries[j] = old_entries[j];','if (old_entries) { T *fresh=entries; for (int j=0; j<length; ++j) fresh[j] = old_entries[j]; }'),
'capacity-after':lambda s:s.replace('capacity = new_capacity;\n        }','capacity = new_capacity;\n        }'),
'local-translation':lambda s:s.replace('T element','const T element'),
}
flags={'nrj1':['-O2','-frename-registers','-fno-reorder-blocks','-falign-jumps=1'],
'nrj4':['-O2','-frename-registers','-fno-reorder-blocks','-falign-jumps=4'],
'nrj8':['-O2','-frename-registers','-fno-reorder-blocks','-falign-jumps=8'],
'nr-no-rename':['-O2','-fno-reorder-blocks','-falign-jumps=4'],
'j4':['-O2','-frename-registers','-falign-jumps=4'],
}
selected_keys=os.environ.get('VUC_INDEX_VARIANTS')
if selected_keys:loops={k:v for k,v in loops.items() if k in selected_keys.split(',')}
selected_adds=os.environ.get('VUC_ADD_VARIANTS')
if selected_adds:addvars={k:v for k,v in addvars.items() if k in selected_adds.split(',')}
rank=[];sizecounts=collections.Counter();n=0
for (name,loop),(addname,transform) in itertools.product(loops.items(),addvars.items()):
 src=base.replace(old,loop)
 src=transform(src)
 cpp=TMP/(name+'-'+addname+'.cpp');cpp.write_text(src)
 for fl,opt in flags.items():
  o=TMP/(name+'-'+addname+'-'+fl+'.o')
  p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opt,'-c',str(cpp),'-o',str(o)],capture_output=True,text=True)
  n+=1
  if p.returncode:
   print('ERROR',name,addname,fl,p.stderr[-80:],flush=True);continue
  sym=next((x for x in subprocess.check_output(['nm','-S',str(o)],text=True).splitlines()
    if x.split()[-1:]==['_Z9TranslateR11ASTNodeListIP7ASTExprE']),'')
  size=int(sym.split()[1],16) if sym else -1;sizecounts[size]+=1
  if size!=256:continue
  try:
   r=m.run(o,ROOT/'vuc/config/translate_expr_list_oct14_probe.json')
   record=(r['exact_bytes'],name,addname,fl,r['different_offsets'][:12],o)
   rank.append(record)
   if r['exact']: print('NEW_EXACT',record,flush=True);raise SystemExit(0)
  except ValueError as e:print('REJECT',name,addname,fl,str(e)[:110],flush=True)
 if n%65==0:print('CHECKPOINT',n,'BEST',max((x[0] for x in rank),default=0),flush=True)
print('TOTAL',n,'LENGTHS',sizecounts.most_common(20),'SCORED',len(rank),flush=True)
print('TOP')
for x in sorted(rank,reverse=True)[:40]:print(x,flush=True)
