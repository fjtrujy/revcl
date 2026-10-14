#!/usr/bin/env python3
"""Authentic i386 ABI/lifetime tests for VUC ASTExpr list reconstruction.

Targets original 256-byte function and two original relocation symbols.
Construct C++03 sources from normal list-append forms; never emit asm or
synthetic bytes. The original's rep movsl copy is the specific target.
"""
from pathlib import Path
import itertools, subprocess, importlib.util, collections
R=Path(__file__).resolve().parents[2]
TMP=Path('/tmp/vuc-binutils215/ast-expr-copy-lifetime-oct14')
TMP.mkdir(parents=True, exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
base=(R/'vuc/probes/translate_expr_list_oct14.cpp').read_text()
mark=base.index('    void Add(T element) {')
end=base.index('\n};',mark)
copy='__builtin_memcpy(entries, old_entries, length * sizeof(T));'
assert copy in base
add=base[mark:end]
variants={}
for copyname,replacement in {
 'memcpy':copy,
 'direct-memcpy':'memcpy(entries, old_entries, length * sizeof(T));',
 'memmove':'__builtin_memmove(entries, old_entries, length * sizeof(T));',
 'bcopy':'__builtin_memcpy((void*)entries, (const void*)old_entries, (unsigned)length << 2);',
 'memcpy-locals':'unsigned bytes=length*4; void *to=entries; const void *from=old_entries; __builtin_memcpy(to,from,bytes);',
 'memcpy-new':'T *fresh=new T[new_capacity]; if (old_entries) __builtin_memcpy(fresh,old_entries,length*sizeof(T)); entries=fresh;',
 'copy-loop':'for (int j=0; j<length; ++j) entries[j] = old_entries[j];',
}.items():
 for order,mode in {
  'basic':('',''),
  'local-length':('int old_size=length;','old_size'),
  'capacity-early':('capacity=new_capacity;',''),
  'assign-after':('',''),
 }.items():
  content=add
  if copyname=='memcpy-new':
   content=content.replace('entries = new T[new_capacity];\n            if (old_entries)\n                '+copy,'')
   content=content.replace('            capacity = new_capacity;', '            '+replacement+'\n            capacity = new_capacity;')
  elif copyname=='direct-memcpy':
   content=content.replace(copy,replacement)
  else:
   content=content.replace(copy,replacement)
  if order=='local-length':
   content=content.replace('if (length >= capacity) {','int old_size=length;\n        if (old_size >= capacity) {').replace('length * sizeof(T)','old_size * sizeof(T)').replace('length*4','old_size*4').replace('entries[length++] = element;','entries[old_size]=element; length=old_size+1;')
  if order=='capacity-early':
   content=content.replace('            T *old_entries = entries;','            capacity = new_capacity;\n            T *old_entries = entries;').replace('            capacity = new_capacity;\n        }','        }')
  if order=='assign-after':
   if copyname!='memcpy-new':
    content=content.replace('entries = new T[new_capacity];','T *fresh = new T[new_capacity];').replace('if (old_entries)','if (old_entries)').replace('memcpy(entries,','memcpy(fresh,').replace('memmove(entries,','memmove(fresh,').replace('void *to=entries','void *to=fresh').replace('entries[j] = old_entries[j]','fresh[j] = old_entries[j]').replace('            capacity = new_capacity;', '            entries=fresh;\n            capacity = new_capacity;')
  variants[copyname+'-'+order]=content
outs={
 'postinc':'int i=0;\n    while(i<input.length)\n        result.Add(Translate(input.entries[i++], 0x3c00, 0));',
 'postinc-volatile':'volatile int i=0;\n    while(i<input.length)\n        result.Add(Translate(input.entries[i++], 0x3c00, 0));',
 'postinc-local':'int i=0;\n    while(i<input.length) {\n       IR::Expr_Data *e=Translate(input.entries[i++], 0x3c00, 0);\n       result.Add(e);\n    }',
 'postinc-volatile-local':'volatile int i=0;\n    while(i<input.length) {\n       IR::Expr_Data *e=Translate(input.entries[i++], 0x3c00, 0);\n       result.Add(e);\n    }',
 'postinc-explicit':'int i=0;\n    while(i<input.length){\n      int index=i; ++i;\n      result.Add(Translate(input.entries[index],0x3c00,0));\n    }',
 'for':'for(int i=0;i<input.length;++i) result.Add(Translate(input.entries[i],0x3c00,0));',
 'for-volatile':'for(volatile int i=0;i<input.length;++i) result.Add(Translate(input.entries[i],0x3c00,0));',
}
compiler='/tmp/vuc-binutils215/g++-jan25-as215'
flags={'nrj1':['-O2','-frename-registers','-falign-jumps=1','-fno-reorder-blocks'],
       'nrj4':['-O2','-frename-registers','-falign-jumps=4','-fno-reorder-blocks'],
       'rj4':['-O2','-frename-registers','-falign-jumps=4'],
       'nrplain':['-O2','-falign-jumps=4','-fno-reorder-blocks']}
sizecounts=collections.Counter();rows=[];attempts=0
for (cn,ab),(on,outer) in itertools.product(variants.items(),outs.items()):
 source=base[:mark]+ab+base[end:]
 source=source.replace('#include <cstring>','')  # builtins provide the intrinsic
 if cn.startswith('direct-memcpy'):
  source='#include <cstring>\n'+source
  source=source.replace('memcpy(', '::memcpy(') if 'memcpy(' in source else source
 function_start=source.index('ASTNodeList<IR::Expr_Data *> Translate(')
 source=source[:function_start]+'''ASTNodeList<IR::Expr_Data *> Translate(ASTNodeList<ASTExpr *> &input)
{
    ASTNodeList<IR::Expr_Data *> result;
    '''+outer+'''
    return result;
}
'''
 key=cn+'--'+on
 cpp=TMP/(key+'.cpp');cpp.write_text(source)
 for label,opts in flags.items():
  o=TMP/(key+'-'+label+'.o')
  p=subprocess.run([compiler,*opts,'-c',str(cpp),'-o',str(o)],capture_output=True,text=True)
  attempts+=1
  if p.returncode:
   print('COMPILE',key,label,p.stderr[-100:],flush=True);continue
  nm=subprocess.check_output(['nm','-S',str(o)],text=True)
  entry=next((x for x in nm.splitlines() if x.split()[-1:]==['_Z9TranslateR11ASTNodeListIP7ASTExprE']),'')
  size=int(entry.split()[1],16) if entry else -1;sizecounts[size]+=1
  if size!=256:continue
  try:
   result=m.run(o,R/'vuc/config/translate_expr_list_oct14_probe.json')
   row=(result['exact_bytes'],key,label,result['different_offsets'][:12],o)
   rows.append(row)
   if result['exact']:
    print('NEW_EXACT',row,flush=True);raise SystemExit(0)
  except ValueError as e:
   print('MATCH_REJECT',key,label,str(e)[:110],flush=True)
 if attempts%56==0:print('BATCH',attempts,'best',max((x[0] for x in rows),default=0),flush=True)
print('TOTAL',attempts,'LENGTHS',sizecounts.most_common(19),'MATCHABLE',len(rows),flush=True)
print('TOP')
for row in sorted(rows,reverse=True)[:30]:print(row,flush=True)
