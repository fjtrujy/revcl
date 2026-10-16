#!/usr/bin/env python3
"""Test prologue ostream return contracts; no synthetic machine code."""
from pathlib import Path
import subprocess,importlib.util
R=Path(__file__).resolve().parents[1];O=Path('/tmp/vuc-prologue-return-oct15v');O.mkdir(exist_ok=True)
base=(R/'probes/asm_output_hypothesis.cpp').read_text();retstart='void AsmOutput_WritePrologue(std::ofstream &out)';end='\n}'
forms={
'reference-return':('std::ostream &','return out << '),
'const-reference-return':('const std::ostream &','return out << '),
'pointer-return':('std::ostream *','return &(out << '),
'void-return':('void','return (void)(out << '),
}
pre,expression=base.split('    out << ',1)
for name,(typ,prefix) in forms.items():
 raw=expression.rsplit(';',1)[0].strip();line=prefix+raw+(');' if name.endswith('return') and name in ('pointer-return','void-return') else ';')
 code=pre.replace(retstart,typ+' AsmOutput_WritePrologue(std::ofstream &out)')+'    '+line+'\n}\n'
 path=O/(name+'.cpp');path.write_text(code)
 for cc in ('jan25',):
  for fn,flags in [('default',['-O2']),('rename',['-O2','-frename-registers'])]:
   obj=O/(name+'-'+cc+'-'+fn+'.o');p=subprocess.run(['/tmp/vuc-binutils215/g++-'+cc+'-as215',*flags,'-c',str(path),'-o',str(obj)],capture_output=True,text=True)
   if p.returncode:print('FAIL',name,fn,p.stderr[-130:],flush=True);continue
   sp=importlib.util.spec_from_file_location('matcher',R/'scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
   try:
    score=m.run(obj,R/'config/asm_output_jan25_probe.json');print('RESULT',name,fn,score['length'],score['exact_bytes'],score['different_offsets'][:16],flush=True)
    if score['exact']:print('BYTE_EXACT',name,fn,flush=True);(O/'EXACT.txt').write_text(name+' '+fn+'\n')
   except Exception as e:print('REJECT',name,fn,str(e)[:170],flush=True)
