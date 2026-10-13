#!/usr/bin/env python3
"""Original-style AsmOutput source-unit context before constant width scanner."""
from pathlib import Path
import importlib.util,subprocess,json
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/tab-constants-asm-unit-oct13');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
stream=(R/'vuc/probes/asm_output_real_unit_oct13.cpp').read_text()
tab=(R/'vuc/probes/asm_output_tab_constants.cpp').read_text()
align=(R/'vuc/probes/align_string.cpp').read_text()
stream=stream[stream.index('#include <fstream>'):]
tab=tab[tab.index('// Source hypothesis'):]
align=align[align.index('#include <cstring>'):]
layouts={
 'stream-tab':stream+'\n'+tab,
 'tab-stream':tab+'\n'+stream,
 'stream-align-tab':stream+'\n'+align+'\n'+tab,
 'align-stream-tab':align+'\n'+stream+'\n'+tab,
 'stream-tab-align':stream+'\n'+tab+'\n'+align,
 'tab-align-stream':tab+'\n'+align+'\n'+stream,
 'align-tab':align+'\n'+tab,
 'tab-align':tab+'\n'+align,
}
config0=json.loads((R/'vuc/config/asm_output_tab_constants_probe.json').read_text())
for name,src in layouts.items():
 p=T/(name+'.cpp');p.write_text(src)
 conf=json.loads(json.dumps(config0));conf['source']=str(p)
 if 'stream' in name and name.startswith('stream'):
  conf['relocation_literals']={'.rodata':[
   {'offset':0,'size':92,'address':'0x080720a0'},
   {'offset':92,'size':19,'address':'0x080721c4'},
   {'offset':111,'size':8,'address':'0x080721f0'},
   {'offset':119,'size':5,'address':'0x080721fb'}]}
 elif name.startswith('align') and 'stream' in name:
  conf['relocation_literals']={'.rodata':[
   {'offset':0,'size':92,'address':'0x080720a0'},
   {'offset':92,'size':19,'address':'0x080721c4'},
   {'offset':111,'size':8,'address':'0x080721f0'},
   {'offset':119,'size':5,'address':'0x080721fb'}]}
 else:
  conf['relocation_literals']={'.rodata':[
   {'offset':0,'size':8,'address':'0x080721f0'},
   {'offset':8,'size':5,'address':'0x080721fb'},
   {'offset':13,'size':92,'address':'0x080720a0'},
   {'offset':105,'size':19,'address':'0x080721c4'}]} if 'stream' in name else config0['relocation_literals']
 cpath=T/(name+'.json');cpath.write_text(json.dumps(conf,indent=2))
 for opt,flags in {'plain':['-O2','-falign-jumps=4'],
                   'rename':['-O2','-frename-registers','-falign-jumps=4'],
                   'functions':['-O2','-falign-jumps=4','-ffunction-sections'],
                   'j8':['-O2','-falign-jumps=8']}.items():
  obj=T/(name+'-'+opt+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(p),'-o',str(obj)],capture_output=True,text=True)
  if cp.returncode:print('COMPILE_FAIL',name,opt,cp.stderr[-100:],flush=True);continue
  try:
   r=m.run(obj,cpath)
   print('MATCH',r['exact_bytes'],r['length'],name,opt,flush=True)
   if r['exact']:print('BYTE-EXACT',name,opt,p,flush=True);raise SystemExit(0)
  except ValueError as e:print('REJECT',name,opt,str(e)[:105],flush=True)
