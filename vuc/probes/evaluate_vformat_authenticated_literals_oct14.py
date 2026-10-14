#!/usr/bin/env python3
"""Original VUC VFormatString: 5 authenticated discontiguous assertion
literals and GCC 3.3.3 source-level reserve/retry reconstruction.

The original ELF string addresses are checked byte-for-byte against the
unchanged ZIP before comparing generated code; each GCC-emitted assertion
literal is independently mapped by its bytes, not by a guessed flat .rodata
base address. Wrong-size candidates are *not* scored/accepted.
"""
from pathlib import Path
import importlib.util,subprocess,json,itertools,collections,os
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/vformat-literal-authenticated-oct14')
T.mkdir(parents=True,exist_ok=True)
def load_module(name,path):
 spec=importlib.util.spec_from_file_location(name,path)
 mod=importlib.util.module_from_spec(spec)
 spec.loader.exec_module(mod)
 return mod
m=load_module('match',R/'vuc/scripts/match_probe.py')
decomp=load_module('original_elf',R/'vuc/scripts/decomp.py')
raw,elf=decomp.load()
expected=[
  ('numWritten >= 0','0x080768c5'),
  ('char* MLTempStringData::VFormatString(const char*, char*)','0x08076860'),
  ('../megalib/MLGameString.cpp','0x080768d5'),
  ('Max() > strLen','0x080768f1'),
  ('char* MLTempStringData::Reserve(int)','0x080768a0'),
]
original_rodata=next(x for x in elf['sections'] if x['name']=='.rodata')
for literal,hexaddr in expected:
 address=int(hexaddr,16)
 b=literal.encode()+b'\0'
 off=original_rodata['offset']+address-original_rodata['address']
 assert raw[off:off+len(b)]==b,(literal,hexaddr)
print('VERIFIED_ORIGINAL_LITERALS',[(k,a) for k,a in expected],flush=True)
base=json.loads((R/'vuc/config/ml_temp_vformat_probe.json').read_text())
assert base['function']=='_ZN16MLTempStringData13VFormatStringEPKcPc'
base['relocations'].pop('.rodata',None)
# Reconstruct the prior, independently semantic source-form experiment's
# definitions without launching the old incomplete relocation validator.
previous=(R/'vuc/probes/evaluate_vformat_reserve_oct14.py').read_text()
definitions=previous[:previous.index('rows=[]')]
scope={'__file__':str(R/'vuc/probes/evaluate_vformat_reserve_oct14.py'),'__name__':'vformat_sources'}
exec(compile(definitions,'vformat_sources','exec'),scope)
forms=scope['forms']
prefix=scope['prefix'];source_header=scope['source_header'];helper=scope['helper']
# Additional genuine source-level reorderings of reserve checks and cursor.
first=scope['initial']
forms['original-check-expression']=first.replace(
 '0x1fbf - (int)used_ < strLen + 1',
 '0x1fbf - (int)used_ < (int)(strlen(format) + 1)')
forms['reserve-needed-first']=first.replace(
 'int strLen = strlen(format);',
 'int strLen = strlen(format);\n    const int required = strLen + 1;'
 ).replace('strLen + 1','required')  # first occurrence is declaration; guard corrected below
forms['reserve-needed-first']=forms['reserve-needed-first'].replace('const int required = required;', 'const int required = strLen + 1;')
forms['reserve-difference-after']=first.replace(
 'int strLen = strlen(format);',
 'int strLen = strlen(format);\n    int remaining = 0x1fbf - (int)used_;'
 ).replace('if (0x1fbf - (int)used_ < strLen + 1)','if (remaining < strLen + 1)')
forms['reserve-rvalue-const']=first.replace(
 'int strLen = strlen(format);',
 'const int strLen = (int)strlen(format);'
)
forms['reserve-used-reference']=first.replace(
 'int strLen = strlen(format);',
 'int strLen = strlen(format);\n    unsigned &used = used_;'
 ).replace('used_','used').replace('unsigned &used = used;','unsigned &used = used_;')
flags={
'r':['-O2','-frename-registers'],
'p':['-O2'],
'r-j1':['-O2','-frename-registers','-falign-jumps=1'],
'r-j2':['-O2','-frename-registers','-falign-jumps=2'],
'r-j4':['-O2','-frename-registers','-falign-jumps=4'],
'r-j8':['-O2','-frename-registers','-falign-jumps=8'],
'p-j1':['-O2','-falign-jumps=1'],
'p-j4':['-O2','-falign-jumps=4'],
'r-noblocks':['-O2','-frename-registers','-fno-reorder-blocks'],
'r-regmove':['-O2','-frename-registers','-fno-regmove'],
}
compilers={'jan25':'/tmp/vuc-binutils215/g++-jan25-as215'}
if os.environ.get('VUC_BOTH')=='1':
 compilers['dec06']='/tmp/vuc-binutils215/g++-dec06-as215'
scores=[];lengths=collections.Counter();n=0;rejects=collections.Counter()
for name,body in forms.items():
 uses_helper=name.startswith('inline-reserve')
 code=source_header+helper+body if uses_helper else prefix+body
 cpp=T/(name+'.cpp');cpp.write_text(code)
 for snapshot,compiler in compilers.items():
  for mode,opts in flags.items():
   obj=T/(name+'-'+snapshot+'-'+mode+'.o')
   cp=subprocess.run([compiler,*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
   n+=1
   if cp.returncode:
    print('COMPILE_ERROR',name,snapshot,mode,cp.stderr[-105:],flush=True);continue
   sym=next((s for s in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines()
    if s.split()[-1:]==['_ZN16MLTempStringData13VFormatStringEPKcPc']),'')
   length=int(sym.split()[1],16) if sym else -1
   lengths[length]+=1
   if length!=238:continue
   binary=obj.read_bytes()
   sections,headers=m.elf_sections(binary)
   if '.rodata' not in sections:
    rejects['no_rodata']+=1;continue
   data=m.section_bytes(binary,sections['.rodata'][1])
   spans=[]
   for literal,hexaddr in expected:
    b=literal.encode()+b'\0'
    pos=data.find(b)
    if pos<0 or data.find(b,pos+1)>=0:
     spans=[];rejects['literal_absent_or_duplicate']+=1;break
    spans.append({'offset':pos,'size':len(b),'address':hexaddr})
   if not spans:continue
   config=dict(base,source=str(cpp),flags=opts,
               relocation_literals={'.rodata':spans})
   manifest=T/(name+'-'+snapshot+'-'+mode+'.json')
   manifest.write_text(json.dumps(config,indent=2)+'\n')
   try:
    res=m.run(obj,manifest)
    scores.append((res['exact_bytes'],name,snapshot,mode,res['different_offsets'][:20],obj,manifest))
    if res['exact']:
     print('NEW_EXACT',name,snapshot,mode,obj,manifest,flush=True)
     raise SystemExit(0)
   except ValueError as e:rejects[str(e)[:110]]+=1
 print('DONE',name,'attempted',n,'scored',len(scores),'best',max((x[0] for x in scores),default=0),flush=True)
print('TESTED',n,'SIZES',lengths.most_common(20),'REJECTS',rejects.most_common(10))
print('TOP')
for row in sorted(scores,reverse=True)[:35]:
 print(row,flush=True)
