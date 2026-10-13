#!/usr/bin/env python3
"""Test genuine strchr character-expression and receiver aliases in VUC AsmOutput source context.

Target: AlignString 0x08049b80, original 145 bytes, 143/145 original
with a direct 0x20 argument. Preserve the independently byte-exact
AsmOutput_TabConstants and verify the source unit's complete 124-byte
rodata section. No artificial source padding or machine-code injection.
"""
from pathlib import Path
import importlib.util, subprocess, json
ROOT = Path(__file__).resolve().parents[2]
TMP = Path('/tmp/vuc-binutils215/align-strchr-receiver-oct13')
TMP.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',ROOT/'vuc/scripts/match_probe.py')
matcher=importlib.util.module_from_spec(spec);spec.loader.exec_module(matcher)
source=(ROOT/'vuc/src/AsmOutputUnit.cpp').read_text()
old_loop='''            int padding = alignment - offset;
            for (int i = 0; i < padding; ++i)
                space[i] = ' ';
            space[padding] = 0;
            strcat(space, text + offset);'''
new_loop='''            int i;
            for (i = 0; i < alignment - offset; ++i) space[i] = ' ';
            space[i] = 0;
            strcat(space, text + (space - formatted));'''
assert old_loop in source
source=source.replace(old_loop,new_loop)
line="    char *space = strchr(formatted, ' ');"
assert line in source
variants={
 'original':line,
 'int-local':"    int character = ' ';\n    char *space = strchr(formatted, character);",
 'char-local':"    char character = ' ';\n    char *space = strchr(formatted, character);",
 'unsigned-char-local':"    unsigned char character = ' ';\n    char *space = strchr(formatted, character);",
 'const-int-local':"    const int character = ' ';\n    char *space = strchr(formatted, character);",
 'const-char-local':"    const char character = ' ';\n    char *space = strchr(formatted, character);",
 'int-pointer-receiver':"    char *receiver = formatted;\n    char *space = strchr(receiver, ' ');",
 'int-pointer-const':"    const char *receiver = formatted;\n    char *space = const_cast<char *>(strchr(receiver, ' '));",
 'function-scoped':"    char *space;\n    { space = strchr(formatted, ' '); }",
 'paren':"    char *space = (strchr(formatted, ' '));",
 'explicit-int':"    char *space = strchr(formatted, static_cast<int>(' '));",
 'explicit-char':"    char *space = strchr(formatted, static_cast<char>(' '));",
 'integer-32':"    char *space = strchr(formatted, 32);",
 'hex-32':"    char *space = strchr(formatted, 0x20);",
 'std-qualify':"    char *space = std::strchr(formatted, ' ');",
 'direct-assign':"    char *space = 0;\n    space = strchr(formatted, ' ');",
 'space-after':"    char *space;\n    space = strchr(formatted, ' ');",
 'conditional':"    char *space = strchr(formatted, ' ');\n    if (space != 0) { /* original nonnull check follows */ }",
 'character-unsigned':"    unsigned character = ' ';\n    char *space = strchr(formatted, character);",
 'volatile-character':"    volatile int character = ' ';\n    char *space = strchr(formatted, character);",
 'local-result':"    char *located = strchr(formatted, ' ');\n    char *space = located;",
}
flags={
 'rename':['-O2','-frename-registers','-falign-jumps=4'],
 'plain':['-O2','-falign-jumps=4'],
 'rename-j1':['-O2','-frename-registers','-falign-jumps=1']
}
full_literals=[{'offset':0,'size':92,'address':'0x080720a0'}, {'offset':92,'size':19,'address':'0x080721c4'}, {'offset':111,'size':8,'address':'0x080721f0'}, {'offset':119,'size':5,'address':'0x080721fb'}]
align_cfg=json.loads((ROOT/'vuc/config/align_string_probe.json').read_text())
align_cfg['relocation_literals']={'.rodata':full_literals}
config_file=TMP/'align-original-literals.json';config_file.write_text(json.dumps(align_cfg))
tab_cfg=ROOT/'vuc/config/asm_output_tab_constants_probe.json'
rank=[]
for name,newline in variants.items():
 src=TMP/(name+'.cpp');src.write_text(source.replace(line,newline))
 for opts_name,opts in flags.items():
  obj=TMP/(name+'-'+opts_name+'.o')
  p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(src),'-o',str(obj)],capture_output=True,text=True)
  if p.returncode:
   print('COMPILER ERROR',name,opts_name,p.stderr[-180:],flush=True)
   continue
  syms=subprocess.run(['nm','-S',str(obj)],capture_output=True,text=True).stdout
  symbol=next((s for s in syms.splitlines() if '_Z11AlignStringPKci' in s),'')
  size=int(symbol.split()[1],16) if symbol else 0
  if size!=145:
   rank.append((0,name,opts_name,size))
   continue
  try:
   result=matcher.run(obj,config_file)
   rank.append((result['exact_bytes'],name,opts_name,size))
   if result['exact']:
    tab=matcher.run(obj,tab_cfg)
    print('ALIGN BYTE EXACT',name,opts_name,src,'TAB EXACT',tab['exact'],tab['exact_bytes'],flush=True)
    raise SystemExit(0)
  except ValueError as exc: print('REJECT',name,opts_name,str(exc)[:125],flush=True)
 print('checked',name,flush=True)
for item in sorted(rank,reverse=True)[:30]:print('BEST',item)
