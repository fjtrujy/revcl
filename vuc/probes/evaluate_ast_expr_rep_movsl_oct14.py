#!/usr/bin/env python3
"""Probe true GCC3.3 inlined string-copy expansion around AST list Add.

Original VUC 256-byte body uses alignment-sensitive rep movsl. Unlike scalar
copy source, genuine builtin memcpy already emits cld; rep movsl with Jan
GCC 3.3.3. Test pointer and copy-count ownership to recover seven missing
bytes of copy/bounds code, without asm, alignment padding or byte patches.
"""
from pathlib import Path
import itertools, subprocess, importlib.util, collections
ROOT=Path(__file__).resolve().parents[2]
TMP=Path('/tmp/vuc-binutils215/ast-expr-rep-movsl-oct14');TMP.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',ROOT/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
base=(ROOT/'vuc/probes/translate_expr_list_partial_improved_oct14.cpp').read_text()
copystart='''            if (old_entries)
                for (int j=0; j<length; ++j) entries[j] = old_entries[j];'''
assert copystart in base
copyforms={
 'memcpy-length':'if (old_entries) __builtin_memcpy(entries, old_entries, length*sizeof(T));',
 'memcpy-oldsize':'if (old_entries) __builtin_memcpy(entries, old_entries, old_size*sizeof(T));',
 'memcpy-count-shift':'if (old_entries) __builtin_memcpy(entries, old_entries, ((unsigned)length)<<2);',
 'memcpy-byte-local':'if (old_entries) { unsigned count=length*sizeof(T); __builtin_memcpy(entries,old_entries,count); }',
 'memcpy-byte-old':'if (old_entries) { unsigned count=old_size*sizeof(T); __builtin_memcpy(entries,old_entries,count); }',
 'memcpy-typed':'if (old_entries) __builtin_memcpy((void *)entries,(const void *)old_entries, (unsigned)length * 4);',
 'memcpy-typed-old':'if (old_entries) __builtin_memcpy((void *)entries,(const void *)old_entries, (unsigned)old_size * 4);',
 'memcpy-destination':'if (old_entries) { T *dst=entries; __builtin_memcpy(dst,old_entries,length*sizeof(T)); }',
 'memcpy-source':'if (old_entries) { T *src=old_entries; __builtin_memcpy(entries,src,length*sizeof(T)); }',
 'memcpy-temp-both':'if (old_entries) { T *dst=entries; const T *src=old_entries; unsigned count=length*sizeof(T); __builtin_memcpy(dst,src,count); }',
 'memcpy-untyped-count':'if (old_entries) { int n=length*4; void *dst=entries; const void *src=old_entries; __builtin_memcpy(dst,src,n); }',
 'memmove-length':'if (old_entries) __builtin_memmove(entries,old_entries,length*sizeof(T));',
 'memmove-oldsize':'if (old_entries) __builtin_memmove(entries,old_entries,old_size*sizeof(T));',
 'std-copy':'if (old_entries) std::copy(old_entries,old_entries+length,entries);',
 'std-copy-old':'if (old_entries) std::copy(old_entries,old_entries+old_size,entries);',
}
orderforms={
 'direct':('entries = new T[new_capacity];',''),
 'new-local':('T *new_entries = new T[new_capacity];\n            entries = new_entries;',''),
 'assign-after':('T *new_entries = new T[new_capacity];','entries = new_entries;'),
 'capacity-before':('capacity=new_capacity;\n            entries = new T[new_capacity];',''),
 'new-with-size':('entries = new T[(unsigned)new_capacity];',''),
 'store-new-local':('T *new_entries = new T[new_capacity];\n            entries = new_entries;',''),
}
flags={
 'nrj1':['-O2','-frename-registers','-fno-reorder-blocks','-falign-jumps=1'],
 'nrj2':['-O2','-frename-registers','-fno-reorder-blocks','-falign-jumps=2'],
 'nrj4':['-O2','-frename-registers','-fno-reorder-blocks','-falign-jumps=4'],
 'nrj8':['-O2','-frename-registers','-fno-reorder-blocks','-falign-jumps=8'],
 'rj4':['-O2','-frename-registers','-falign-jumps=4'],
 'plainj4':['-O2','-fno-reorder-blocks','-falign-jumps=4'],
}
rows=[];sizecounts=collections.Counter();n=0
for (copyname,copy),(ordname,(newexpr,final)) in itertools.product(copyforms.items(),orderforms.items()):
 text=base.replace(copystart,'            '+copy)
 if copyname.startswith('std-copy'):text='#include <algorithm>\n'+text
 if ordname=='capacity-before':
  text=text.replace('            capacity = new_capacity;\n        }','        }')
 if ordname=='assign-after':
  # Copy to freshly allocated destination before making it visible in list.
  text=text.replace('            '+copy, '            '+copy.replace('memcpy(entries,','memcpy(new_entries,').replace('memmove(entries,','memmove(new_entries,').replace('entries,old_entries','new_entries,old_entries').replace('entries, old_entries','new_entries, old_entries').replace('dst=entries','dst=new_entries').replace('std::copy(old_entries,old_entries+length,entries)','std::copy(old_entries,old_entries+length,new_entries)').replace('std::copy(old_entries,old_entries+old_size,entries)','std::copy(old_entries,old_entries+old_size,new_entries)'))
 if final:
  text=text.replace('            capacity = new_capacity;\n        }','            '+final+'\n            capacity = new_capacity;\n        }')
 text=text.replace('            entries = new T[new_capacity];', '            '+newexpr)
 key=copyname+'-'+ordname
 cpp=TMP/(key+'.cpp');cpp.write_text(text)
 for flag,opts in flags.items():
  obj=TMP/(key+'-'+flag+'.o')
  p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
  n+=1
  if p.returncode:
   print('COMPILER_ERROR',key,flag,p.stderr[-125:],flush=True);continue
  nm=subprocess.check_output(['nm','-S',str(obj)],text=True)
  symbol=next((x for x in nm.splitlines() if x.split()[-1:]==['_Z9TranslateR11ASTNodeListIP7ASTExprE']),'')
  size=int(symbol.split()[1],16) if symbol else -1;sizecounts[size]+=1
  if size!=256:continue
  try:
   res=m.run(obj,ROOT/'vuc/config/translate_expr_list_oct14_probe.json')
   row=(res['exact_bytes'],key,flag,res['different_offsets'][:12],str(obj))
   rows.append(row)
   if res['exact']:print('NEW_EXACT',row,flush=True);raise SystemExit(0)
  except ValueError as e:print('REJECT',key,flag,str(e)[:115],flush=True)
 if n%90==0:print('CHECKPOINT',n,'BEST',max((a[0] for a in rows),default=0),'VALID',len(rows),flush=True)
print('TESTED',n,'LENGTHS',sizecounts.most_common(21),'SCORED',len(rows),flush=True)
print('TOP')
for row in sorted(rows,reverse=True)[:40]:print(row,flush=True)
