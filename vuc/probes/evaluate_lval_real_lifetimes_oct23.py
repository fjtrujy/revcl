#!/usr/bin/env python3
"""Source-level lifetime/growth controls for the authentic VUC ASTLValExpr printer.

Unlike simple aliases, preloads are *actually consumed after* a potentially
throwing container allocation, altering the true compiler liveness graph.
Do not treat equal lengths as matches: the matcher verifies original ELF
symbol, relocation targets, and byte-authenticated literal slices.
"""
from pathlib import Path
import itertools,importlib.util,json,re,subprocess,hashlib
R=Path(__file__).resolve().parents[1]
OUT=Path('/tmp/vuc-ast-lval-lifetimes-20261023')
OUT.mkdir(exist_ok=True)
source=(R/'probes/PrintASTLValRecursionOwnerOct23.cpp').read_text()
source=source.replace('#include "../src/MLGameString.h"',
 '#include "'+str(R/'src/MLGameString.h')+'"')
spec=importlib.util.spec_from_file_location('auth',R/'scripts/match_probe.py')
auth=importlib.util.module_from_spec(spec);spec.loader.exec_module(auth)
baseadd=source.index('    void Add(ASTNode *node) {')
endadd=source.index('\n    void Pop()',baseadd)
oldadd=source[baseadd:endadd]
basevariants={
 'baseline':oldadd,
 'start-save':oldadd.replace('int previous = length;', 'int previous = length; ASTNode **const saved = entries;').replace('ASTNode **oldEntries = entries;', 'ASTNode **oldEntries = saved;'),
 'count-after':oldadd.replace('entries[previous] = node;\n        length = previous + 1;', 'entries[length++] = node;'),
 'count-before':oldadd.replace('entries[previous] = node;\n        length = previous + 1;', 'length = previous + 1;\n        entries[previous] = node;'),
 'test-equality':oldadd.replace('previous >= capacity','previous == capacity'),
 'copy-destination-local':oldadd.replace('entries = new ASTNode *[expanded];\n            if (oldEntries)\n                __builtin_memcpy(entries, oldEntries, previous * sizeof(ASTNode *));',
    'ASTNode **newEntries = new ASTNode *[expanded];\n            entries = newEntries;\n            if (oldEntries)\n                __builtin_memcpy(newEntries, oldEntries, previous * sizeof(ASTNode *));'),
 'copy-prestore':oldadd.replace('entries = new ASTNode *[expanded];\n            if (oldEntries)\n                __builtin_memcpy(entries, oldEntries, previous * sizeof(ASTNode *));',
    'ASTNode **newEntries = new ASTNode *[expanded];\n            if (oldEntries)\n                __builtin_memcpy(newEntries, oldEntries, previous * sizeof(ASTNode *));\n            entries = newEntries;'),
 'allocate-array-temp':oldadd.replace('entries = new ASTNode *[expanded];', 'ASTNode **newEntries = new ASTNode *[expanded];\n            entries = newEntries;'),
 'capacity-ternary':oldadd.replace('int expanded = capacity ? 2 * capacity : 16;', 'int expanded = capacity == 0 ? 16 : capacity + capacity;'),
}
for name,v in basevariants.items():
 assert 'void Add(ASTNode *node)' in v and 'entries[' in v,name
expectedcase1='''        ScopedPrintNode entry(node);
        ASTLValArrayExpr *array = static_cast<ASTLValArrayExpr *>(node);
        Print(array->base, output);
        output << "[";
        Print(array->index, output);
        output << "]";'''
expectedcase2='''        ScopedPrintNode entry(node);
        ASTLValMemExpr *member = static_cast<ASTLValMemExpr *>(node);
        Print(member->base, output);
        output << ".";
        Print(member->member, output);'''
assert source.count(expectedcase1)==source.count(expectedcase2)==1
contexts={}
contexts['late']=source
# The operands' *values* are read and saved before guard Add, then used
# as the actual eventual recursive-call arguments.
for mode in ('both','first','second'):
 c=source
 pre1='''        ASTLValArrayExpr *array = static_cast<ASTLValArrayExpr *>(node);
'''
 pre2='''        ASTLValMemExpr *member = static_cast<ASTLValMemExpr *>(node);
'''
 if mode in ('both','first'):
  pre1+='        ASTLValExpr *arrayBase = array->base;\n'
  pre2+='        ASTLValExpr *memberBase = member->base;\n'
 if mode in ('both','second'):
  pre1+='        ASTExpr *arrayIndex = array->index;\n'
  pre2+='        ASTLValExpr *memberName = member->member;\n'
 after1='''        ScopedPrintNode entry(node);
        Print('''+('arrayBase' if mode in ('both','first') else 'array->base')+''', output);
        output << "[";
        Print('''+('arrayIndex' if mode in ('both','second') else 'array->index')+''', output);
        output << "]";'''
 after2='''        ScopedPrintNode entry(node);
        Print('''+('memberBase' if mode in ('both','first') else 'member->base')+''', output);
        output << ".";
        Print('''+('memberName' if mode in ('both','second') else 'member->member')+''', output);'''
 c=c.replace(expectedcase1,pre1+after1).replace(expectedcase2,pre2+after2)
 contexts[mode]=c
# Also use identical preload on only a *single* switch arm. The position of
# live values must be genuinely meaningful to the EBP-relative spill frame.
for mode in ('both','first','second'):
 for changed in ('array','member'):
  c=source
  variant=contexts[mode]
  if changed=='array':
   c=c.replace(expectedcase1,variant[variant.index('        ASTLValArrayExpr *array =',variant.index('    case 1:')):variant.index('        break;',variant.index('    case 1:'))].rstrip())
  else:
   c=c.replace(expectedcase2,variant[variant.index('        ASTLValMemExpr *member =',variant.index('    case 2:')):variant.index('        break;',variant.index('    case 2:'))].rstrip())
  contexts[mode+'-'+changed]=c
print('SCOPE contexts',len(contexts),'ADD forms',len(basevariants),flush=True)
records=[]
for i,((growth,gbody),(liveness,csource)) in enumerate(itertools.product(basevariants.items(),contexts.items())):
 label=growth+'__'+liveness
 new=csource.replace(oldadd,gbody)
 if new==csource and growth!='baseline':raise ValueError('did not replace growth: '+label)
 filename=OUT/(label+'.cpp')
 filename.write_text(new)
 record={'case':label,'sha256':hashlib.sha256(new.encode()).hexdigest()}
 for snap in ('jan25','dec06'):
  obj=OUT/(label+'-'+snap+'.o')
  command=['/tmp/vuc-binutils215/g++-'+snap+'-as215','-O2','-c',str(filename),'-o',str(obj)]
  cp=subprocess.run(command,capture_output=True,text=True)
  if cp.returncode:record[snap]={'error':cp.stderr[-500:]};continue
  # Determine length from nm before strict comparator.
  nm=subprocess.check_output(['nm','-S',str(obj)],text=True)
  match=next((line.split() for line in nm.splitlines()
    if line.endswith('_Z5PrintP11ASTLValExprRSo')),None)
  size=int(match[1],16) if match else 0
  rec={'size':size}
  if size==765:
   try:rec.update(auth.run(obj,R/'config/print_ast_lval_recursion_oct23_probe.json'))
   except Exception as exc:rec['match_error']=str(exc)
   print('ORIGINAL_LENGTH',label,snap,
     rec.get('exact_bytes','REJECT'),rec.get('match_error',''),flush=True)
  record[snap]=rec
 records.append(record)
 if len(records)%16==0:print('completed',len(records),flush=True)
(OUT/'results.json').write_text(json.dumps(records,indent=2)+'\n')
for snap in ('jan25','dec06'):
 valid=[r[snap] for r in records if 'size' in r.get(snap,{})]
 exact=[r[snap] for r in records if r.get(snap,{}).get('exact')]
 from collections import Counter
 print('SUMMARY',snap,'valid',len(valid),'original_size',sum(x['size']==765 for x in valid),
       'exact',len(exact),'sizes',dict(sorted(Counter(x['size'] for x in valid).items())),flush=True)
