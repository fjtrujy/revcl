#!/usr/bin/env python3
"""Original SymbolTable TU: hash-table C++03 EH declaration ownership.

The candidate already emits original loop bytes and original 0x804 stride.
In real owner context it is 145 (j1) or 147 (j4), original 149.
Test actual source declaration ownership for template destructor and clear()
instead of guessing branches or inserting NOPs. Keep all generated
headers and compiler ELF32 objects outside Git under the Lima guest.
"""
from pathlib import Path
import subprocess,json
R=Path(__file__).resolve().parents[1]
O=Path('/tmp/vuc-binutils215/scope-iterator-original-table-owner-oct15t');O.mkdir(exist_ok=True)
start=(R/'src/MLHashTable.h').read_text()
pre=(R/'src/SymbolTableBeginScope.cpp').read_text()
assert '#include "MLHashTable.h"' in pre
cases={
 'original':start,
 'dtor-nothrow':start.replace('~MLHashTable() { clear(); }','~MLHashTable() throw() { clear(); }'),
 'clear-nothrow':start.replace('void clear();','void clear() throw();'),
 'both-nothrow':start.replace('~MLHashTable() { clear(); }','~MLHashTable() throw() { clear(); }').replace('void clear();','void clear() throw();'),
 'copy-nothrow':start.replace('MLHashTable(const MLHashTable &other);','MLHashTable(const MLHashTable &other) throw();'),
 'ctor-nothrow':start.replace('MLHashTable() {','MLHashTable() throw() {'),
 'dtor-this':start.replace('~MLHashTable() { clear(); }','~MLHashTable() { this->clear(); }'),
 'dtor-void':start.replace('~MLHashTable() { clear(); }','~MLHashTable() { (void)clear(); }'),
}
records=[]
for tag,h in cases.items():
 path=O/(tag+'.h');path.write_text(h)
 src=pre.replace('#include "MLHashTable.h"','#include "'+str(path)+'"').replace('#include "MLHashFunGameString.h"','#include "'+str(R/'src/MLHashFunGameString.h')+'"')
 src+='''\n#include <bits/stl_uninitialized.h>
typedef std::vector<Scope> ScopeVector;
typedef ScopeVector::iterator ScopeIter;
template ScopeIter std::__uninitialized_copy_aux<ScopeIter,ScopeIter>(
 ScopeIter,ScopeIter,ScopeIter,__false_type);
'''
 cpp=O/(tag+'.cpp');cpp.write_text(src)
 for snap in ('jan25','dec06'):
  for jump in ('1','4','8'):
   obj=O/(tag+'-'+snap+'-j'+jump+'.o')
   p=subprocess.run(['/tmp/vuc-binutils215/g++-'+snap+'-as215','-O2','-frename-registers','-falign-jumps='+jump,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
   row={'source':tag,'compiler':snap,'jumps':jump}
   if p.returncode:row['error']=p.stderr[-120:]
   else:
    nm=subprocess.run(['nm','-S','--defined-only',str(obj)],capture_output=True,text=True)
    sy=[x.split() for x in nm.stdout.splitlines() if len(x.split())>=4]
    for name,prefix in [('copy','_ZSt24__uninitialized_copy_auxIN9__gnu_cxx'),('insert','_ZNSt6vectorI11MLHashTable')]:
     s=next((x for x in sy if x[-1].startswith(prefix) and (name!='insert' or '_M_insert_aux' in x[-1])),None)
     row[name]=int(s[1],16) if s else -1
   records.append(row);print(row,flush=True)
(O/'results.json').write_text(json.dumps(records,indent=2)+'\n')
