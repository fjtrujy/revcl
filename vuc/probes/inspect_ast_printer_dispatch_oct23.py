#!/usr/bin/env python3
"""Recover VUC original Printer.cpp AST dispatch from the canonical ZIP's ELF.

This is read-only evidence: original symbol sizes, direct indexed switch
targets, selector member offsets, and shared matchStack. Does not manufacture
a reconstructed C++ function or relax the byte-exact acceptance contract.
"""
from pathlib import Path
import importlib.util,json,struct
ROOT=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('vuc_original',ROOT/'vuc/scripts/decomp.py')
d=importlib.util.module_from_spec(sp);sp.loader.exec_module(d)
raw,info=d.load()
secs=info['sections']
def at(address,size):
  matching=[sec for sec in secs if sec['address']<=address and address+size<=sec['address']+sec['size'] and sec['type']!=8]
  if len(matching)!=1:raise ValueError('ambiguous ELF address '+hex(address))
  sec=matching[0]
  offset=sec['offset']+address-sec['address']
  return raw[offset:offset+size]
by_address={f['address']:f for f in info['functions']}
target=[
  ('ASTStmt',0x0805b7e0,2231,8,0x08072fb0,10),
  ('ASTExpr',0x0805c0a0,1089,8,0x08072fd8,6),
  ('ASTDecl',0x0805c7f0,844,8,0x08072ff0,5),
]
records=[]
for typename,address,size,offset,table,n in target:
  f=by_address[address]
  assert f['size']==size
  switch_targets=list(struct.unpack('<'+str(n)+'I',at(table,n*4)))
  assert all(address<=dest<address+size for dest in switch_targets)
  records.append({'type':typename,'start':hex(address),'size':size,
     'selector_offset':hex(offset),'indexed_jump_table':hex(table),
     'selector_addend':1,'cases':[{'selector':i-1,'target':hex(v),
          'relative':hex(v-address)} for i,v in enumerate(switch_targets)]})
lval=by_address[0x0805c4f0]
assert lval['size']==765
records.append({'type':'ASTLValExpr','start':'0x805c4f0','size':765,
 'selector_offset':'0x1c','cases':[
   {'selector':0,'target':'0x805c720','payload_offset':'0x24','interpretation':'MLGameString::c_str() identifier'},
   {'selector':1,'target':'0x805c620','payload_offsets':['0x24','0x28'],'interpretation':'LVal base + expression index, literals [ and ]'},
   {'selector':2,'target':'0x805c53d','payload_offsets':['0x24','0x28'],'interpretation':'two LVal operands, literal .'}]})
stack=next(s for s in info['symbols'] if s['name']=='matchStack' and s['type']==1 and s['address']==0x0808abe4)
assert stack['size']==12
for symbol in records: symbol['shared_matchStack']={'symbol':'matchStack','address':hex(stack['address']),'size':12,
 'count_offset':0,'capacity_offset':4,'entries_offset':8}
print(json.dumps({'original_sha256':info['sha256'],'records':records},indent=2))
