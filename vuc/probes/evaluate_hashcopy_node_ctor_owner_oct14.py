#!/usr/bin/env python3
"""Test plausible VUC hash Node copy constructors and pointer lifetimes.

Original hash copy constructor (0x0804f420, 166 bytes) copies an
MLGameString key, 32-bit hash/value, and singly linked next pointer. This
uses genuine C++03 copy/member-ctor definitions and natural pointer loops,
checking all four original call/unwind relocations. No inline register asm.
"""
from pathlib import Path
import importlib.util,itertools,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/'vuc/config/hash_copy_constructor_probe.json').read_text())
raw,elf=m.decomp.load();section=next(x for x in elf['sections'] if x['name']=='.text')
addr=int(cfg['address'],0);ref=raw[section['offset']+addr-section['address']:section['offset']+addr-section['address']+166]
T=Path('/tmp/vuc-binutils215/hash-copy-node-ctor-lifetimes-next');T.mkdir(parents=True,exist_ok=True)
header=(R/'vuc/src/MLHashTable.h').read_text()
needle='''        Node *next;
    };'''
assert needle in header
copy_ctors={
 'implicit':'',
 'explicit-copy':'        Node(const Node &other) : key(other.key), hash(other.hash), value(other.value), next(other.next) {}\n',
 'explicit-copy-body':'        Node(const Node &other) : key(other.key) { hash = other.hash; value = other.value; next = other.next; }\n',
 'args':'        Node(const Key &k, unsigned h, Value v, Node *n) : key(k), hash(h), value(v), next(n) {}\n',
}
outer=(R/'vuc/probes/hash_copy_constructor.cpp').read_text()
outer=outer.replace('#include "../src/MLHashFunGameString.h"','#include "'+str(R/'vuc/src/MLHashFunGameString.h')+'"')
inner='''        for (Node *node = other.heads[i]; node; node = node->next) {
            Node *copy = new Node(*node);
            copy->next = 0;
            *tail = copy;
            tail = &copy->next;
        }'''
assert inner in outer
styles={
 'baseline':inner,
 'store-first':inner.replace('copy->next = 0;\n            *tail = copy;', '*tail = copy;\n            copy->next = 0;'),
 'tail-first':inner.replace('copy->next = 0;\n            *tail = copy;\n            tail = &copy->next;', 'Node **nextTail = &copy->next;\n            *tail = copy;\n            copy->next = 0;\n            tail = nextTail;'),
 'while-loop':'''        Node *node = other.heads[i];
        while (node) {
            Node *copy = new Node(*node);
            copy->next = 0;
            *tail = copy;
            tail = &copy->next;
            node = node->next;
        }''',
 'pre-next':'''        Node *node = other.heads[i];
        while (node) {
            Node *next = node->next;
            Node *copy = new Node(*node);
            copy->next = 0;
            *tail = copy;
            tail = &copy->next;
            node = next;
        }''',
 'skip-empty':'''        Node *node = other.heads[i];
        if (node) do {
            Node *copy = new Node(*node);
            copy->next = 0;
            *tail = copy;
            tail = &copy->next;
            node = node->next;
        } while (node);''',
}
flags={
 'o2r':['-O2','-frename-registers'],
 'j1':['-O2','-frename-registers','-falign-jumps=1'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
 'l4':['-O2','-frename-registers','-falign-labels=4'],
 'l8':['-O2','-frename-registers','-falign-labels=8'],
}
rows=[];best=None;lengths=collections.Counter()
for (ck,ctor),(innerkey,innerbody),(fl,opts) in itertools.product(copy_ctors.items(),styles.items(),flags.items()):
 tag=f'{ck}-{innerkey}-{fl}'
 if ck=='args':
  innerbody=innerbody.replace('new Node(*node)','new Node(node->key, node->hash, node->value, node->next)')
 # Header is independently scratch-owned; no change to accepted original header.
 headfile=T/(tag+'.h');headfile.write_text(header.replace(needle,'        Node *next;\n'+ctor+'    };'))
 source=outer.replace('#include "../src/MLHashTable.h"','#include "'+str(headfile)+'"').replace(inner,innerbody)
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(source)
 cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if cp.returncode:rows.append(dict(tag=tag,error=cp.stderr[-150:]));continue
 try:body,rel=m.prepare_named_function(obj.read_bytes(),cfg['function'],cfg['relocations'],addr)
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)));continue
 match=sum(a==b for a,b in zip(body,ref))
 prefix=next((i for i,(a,b) in enumerate(zip(body,ref)) if a!=b),min(len(body),len(ref)))
 row=dict(tag=tag,length=len(body),relocations=rel,overlap=match,prefix=prefix)
 rows.append(row);lengths[len(body)]+=1
 if len(body)==166 and rel==4 and (best is None or (match,prefix)>(best['overlap'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(source);(T/'best.h').write_text(headfile.read_text());(T/'best.flags').write_text(' '.join(opts)+'\n');(T/'best.o').write_bytes(obj.read_bytes())
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(lengths),best_original_length=best,top=sorted((x for x in rows if 'overlap' in x),key=lambda x:(x['overlap'],x['prefix']),reverse=True)[:9]),indent=2))
