#!/usr/bin/env python3
"""Recover AlignString as a real AsmOutput source neighbor without breaking exact TabConstants."""
from pathlib import Path
import subprocess,importlib.util
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/align-real-unit-oct13');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
source=(R/'vuc/src/AsmOutputUnit.cpp').read_text()
old='''            int padding = alignment - offset;
            for (int i = 0; i < padding; ++i)
                space[i] = ' ';
            space[padding] = 0;
            strcat(space, text + offset);'''
assert old in source
alternatives={
 'baseline':old,
 'index-terminator':old.replace('            space[padding] = 0;', '            space[padding] = 0;').replace('text + offset','text + (space - formatted)'),
 'for-after':'''            int padding = alignment - offset;
            int i;
            for (i = 0; i < padding; ++i) {
                space[i] = ' ';
            }
            space[i] = 0;
            strcat(space, text + (space - formatted));''',
 'for-after-original-text':'''            int padding = alignment - offset;
            int i;
            for (i = 0; i < padding; ++i) space[i] = ' ';
            space[i] = 0;
            strcat(space, text + offset);''',
 'while-counter':'''            int padding = alignment - offset;
            int i = 0;
            while (i < padding) {
                space[i] = ' ';
                ++i;
            }
            space[i] = 0;
            strcat(space, text + (space - formatted));''',
 'while-increment':'''            int padding = alignment - offset;
            int i = 0;
            while (i < padding) {
                space[i++] = ' ';
            }
            space[i] = 0;
            strcat(space, text + (space - formatted));''',
 'for-comparison-offset':'''            int i;
            for (i = 0; i < alignment - offset; ++i) space[i] = ' ';
            space[i] = 0;
            strcat(space, text + (space - formatted));''',
 'for-temp':'''            int padding = alignment - offset;
            int i=0;
            for (; i < padding; i++) space[i] = ' ';
            space[i] = 0;
            strcat(space, text + (space - formatted));''',
 'for-zero':'''            int padding = alignment - offset;
            int i = 0;
            for (; i != padding; ++i) space[i] = ' ';
            space[i] = 0;
            strcat(space, text + (space - formatted));''',
 'pointer-local':'''            int padding = alignment - offset;
            char *out = space;
            for (int i = 0; i < padding; ++i) out[i] = ' ';
            out[padding] = 0;
            strcat(out, text + (space - formatted));''',
 'direct-width':'''            int i = 0;
            int needed = alignment - (space - formatted);
            while (i < needed) {
                space[i] = ' ';
                ++i;
            }
            space[i] = 0;
            strcat(space, text + (space - formatted));''',
 'unsigned-index':'''            int padding = alignment - offset;
            unsigned i=0;
            for (; i < (unsigned)padding; ++i) space[i] = ' ';
            space[i] = 0;
            strcat(space, text + (space - formatted));''',
 'index-counter':'''            int padding = alignment - offset;
            int i = 0;
            for (; i < padding; i++) space[i] = ' ';
            space[i] = 0;
            strcat(space, text + offset);''',
 'nested-width':'''            int padding = alignment - offset;
            for (int i = 0; i < padding; ++i) space[i] = ' ';
            space[padding] = 0;
            const char *tail = text + (space - formatted);
            strcat(space, tail);''',
}
opts={'r':['-O2','-frename-registers','-falign-jumps=4'], 'plain':['-O2','-falign-jumps=4'], 'j1':['-O2','-frename-registers','-falign-jumps=1'], 'j8':['-O2','-frename-registers','-falign-jumps=8']}
rank=[]
for name,fragment in alternatives.items():
 path=T/(name+'.cpp');path.write_text(source.replace(old,fragment))
 for option,flags in opts.items():
  obj=T/(name+'-'+option+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(path),'-o',str(obj)],capture_output=True,text=True)
  if cp.returncode:print('COMPILE_FAIL',name,option,cp.stderr[-130:],flush=True);continue
  syms=subprocess.run(['nm','-S',str(obj)],capture_output=True,text=True).stdout
  sym=next((x for x in syms.splitlines() if '_Z11AlignStringPKci' in x),'')
  size=int(sym.split()[1],16) if sym else 0
  if size==145:
   try:
    score=m.run(obj,R/'vuc/config/align_string_probe.json')
    rank.append((score['exact_bytes'],name,option,size))
    if score['exact']:
     print('ALIGN BYTE EXACT',name,option,path,flush=True)
     print('TAB STATUS',m.run(obj,R/'vuc/config/asm_output_tab_constants_probe.json')['exact'],flush=True)
     raise SystemExit(0)
   except ValueError as e:print('MATCH_FAIL',name,option,str(e)[:100],flush=True)
  else:rank.append((0,name,option,size))
 print('tested',name,flush=True)
for r in sorted(rank,reverse=True)[:30]:print('BEST',r)
