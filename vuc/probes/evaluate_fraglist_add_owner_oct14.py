#!/usr/bin/env python3
"""GCC 3.3.3 ASTNodeList::Add inline ownership and lifetime reconstruction.

The VUC original fragment-list symbol is 306 bytes and its inlined list
growth is apparent from the original ELF. Test genuine C++03 lifetime and
copy-order spellings, preserving all six authenticated relocations.
All compiled objects are VM scratch, never synthetic binary data.
"""
from pathlib import Path
import itertools,importlib.util,subprocess,json,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
config=json.loads((R/'vuc/config/translate_frag_list_string_oct14_probe.json').read_text())
binary,elf=m.decomp.load();sec=next(s for s in elf['sections'] if s['name']=='.text')
address=int(config['address'],0)
orig=binary[sec['offset']+address-sec['address']:sec['offset']+address-sec['address']+306]
source=(R/'vuc/probes/translate_frag_list_string_partial_oct14.cpp').read_text()
source=source.replace('"../src/MLGameString.h"','"'+str(R/'vuc/src/MLGameString.h')+'"')
start=source.index('    void Add(T element) {')
end=source.index('\n};',start)
old_add=source[start:end]
assert old_add.count('__builtin_memcpy')==1
variants={
 'baseline':old_add,
 'oldsize-scope':old_add.replace('int old_size = length;\n        if', 'int old_size;\n        old_size = length;\n        if'),
 'oldsize-const':old_add.replace('int old_size = length;', 'const int old_size = length;'),
 'oldsize-unsigned':old_add.replace('int old_size = length;', 'unsigned old_size = length;'),
 'capacity-intlocal':old_add.replace('int next_capacity = capacity ? capacity * 2 : 16;', 'int next_capacity = 16;\n            if (capacity) next_capacity = capacity * 2;'),
 'capacity-ternary':old_add.replace('int next_capacity = capacity ? capacity * 2 : 16;', 'int next_capacity = capacity ? (capacity << 1) : 16;'),
 'capacity-if':old_add.replace('int next_capacity = capacity ? capacity * 2 : 16;', 'int next_capacity;\n            if (capacity) next_capacity = 2 * capacity;\n            else next_capacity = 16;'),
 'capacity-unsigned':old_add.replace('int next_capacity = capacity ? capacity * 2 : 16;', 'unsigned next_capacity = capacity ? 2 * capacity : 16;'),
 'oldentries-const':old_add.replace('T *old_entries = entries;', 'T *const old_entries = entries;'),
 'oldentries-after-new':old_add.replace('T *old_entries = entries;\n            entries = new T[next_capacity];', 'T *old_entries = entries;\n            T *new_entries = new T[next_capacity];\n            entries = new_entries;'),
 'defer-store':old_add.replace('entries = new T[next_capacity];\n            if (old_entries)\n                __builtin_memcpy(entries, old_entries, length * sizeof(T));', 'T *new_entries = new T[next_capacity];\n            if (old_entries)\n                __builtin_memcpy(new_entries, old_entries, length * sizeof(T));\n            entries = new_entries;'),
 'defer-capacity':old_add.replace('capacity = next_capacity;', 'capacity = next_capacity;'),
 'copy-oldsize':old_add.replace('length * sizeof(T)', 'old_size * sizeof(T)'),
 'copy-count-bytes':old_add.replace('length * sizeof(T)', 'sizeof(T) * length'),
 'copy-size-unsigned':old_add.replace('length * sizeof(T)', '(unsigned)length * sizeof(T)'),
 'copy-memmove':old_add.replace('__builtin_memcpy(', '__builtin_memmove('),
 'new-with-sizeof':old_add.replace('new T[next_capacity]', 'new T[(unsigned)next_capacity]'),
 'count-before-store':old_add.replace('entries[old_size] = element;\n        length = old_size + 1;', 'length = old_size + 1;\n        entries[old_size] = element;'),
 'postlength':old_add.replace('length = old_size + 1;', '++length;'),
 'copy-guard':old_add.replace('if (old_entries)', 'if (old_entries != 0)'),
 'copy-guard-length':old_add.replace('if (old_entries)', 'if (old_entries && old_size > 0)'),
 'copy-guard-count':old_add.replace('if (old_entries)', 'if (old_size > 0 && old_entries)'),
 'capacity-prestore':old_add.replace('entries = new T[next_capacity];', 'capacity = next_capacity;\n            entries = new T[next_capacity];').replace('            capacity = next_capacity;\n        }', '        }'),
 'capacity-compare':old_add.replace('if (old_size >= capacity)', 'if (length == capacity)'),
 'pointer-assign':old_add.replace('entries[old_size] = element;', '*(entries + old_size) = element;'),
}
flags={
 'j1':['-O2','-frename-registers','-falign-jumps=1'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
 'j1-nr':['-O2','-frename-registers','-falign-jumps=1','-fno-reorder-blocks'],
 'j4-nr':['-O2','-frename-registers','-falign-jumps=4','-fno-reorder-blocks'],
 'no-regmove':['-O2','-frename-registers','-falign-jumps=1','-fno-regmove'],
 'no-cse-follow':['-O2','-frename-registers','-falign-jumps=1','-fno-cse-follow-jumps'],
}
T=Path('/tmp/vuc-binutils215/fraglist-add-owner-next-oct14')
T.mkdir(parents=True,exist_ok=True)
scores=[];counts=collections.Counter();best=None
for (name,variant),(opt,opts) in itertools.product(variants.items(),flags.items()):
 cpp=T/(name+'-'+opt+'.cpp');obj=T/(name+'-'+opt+'.o')
 cpp.write_text(source[:start]+variant+source[end:])
 compiled=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if compiled.returncode:
  scores.append(dict(name=name,flags=opt,error=compiled.stderr[-130:]));continue
 try:code,rels=m.prepare_named_function(obj.read_bytes(),config['function'],config['relocations'],address)
 except ValueError as ex:
  scores.append(dict(name=name,flags=opt,error=str(ex)));continue
 overlap=sum(a==b for a,b in zip(code,orig))
 prefix=next((i for i,(a,b) in enumerate(zip(code,orig)) if a!=b),min(len(code),len(orig)))
 row=dict(name=name,flags=opt,length=len(code),relocations=rels,overlap=overlap,prefix=prefix)
 scores.append(row);counts[len(code)]+=1
 if len(code)==306 and rels==6 and (best is None or (overlap,prefix)>(best['overlap'],best['prefix'])):
  best=row
  (T/'best.cpp').write_text(cpp.read_text());(T/'best.o').write_bytes(obj.read_bytes())
(T/'results.json').write_text(json.dumps(scores,indent=2)+'\n')
print(json.dumps(dict(attempted=len(scores),lengths=dict(counts),best_original_length=best,top=sorted((s for s in scores if 'overlap'in s),key=lambda s:(s['overlap'],s['prefix']),reverse=True)[:12]),indent=2))
