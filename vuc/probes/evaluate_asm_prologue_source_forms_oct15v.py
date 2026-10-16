#!/usr/bin/env python3
"""Genuine C++03 expression and GCC scheduler controls for the 30-byte prologue."""
from pathlib import Path
import subprocess,json
R=Path(__file__).resolve().parents[1];O=Path('/tmp/vuc-asm-prologue-oct15v');O.mkdir(exist_ok=True)
source=(R/'probes/asm_output_hypothesis.cpp').read_text()
literal=source.split('    out << ',1)[1].rsplit(';\n}',1)[0].strip()
forms={
 'direct': 'out << L;',
 'qualified': 'std::operator<<(out, L);',
 'pointer-local': 'const char *text=L; out << text;',
 'pointer-volatile': 'const char *const text=L; out << text;',
 'ostream-ref': 'std::ostream &stream=out; stream << L;',
 'ostream-pointer': 'std::ostream *stream=&out; (*stream) << L;',
 'ofstream-pointer': 'std::ofstream *stream=&out; *stream << L;',
 'explicit-ostream': 'std::operator<<(static_cast<std::ostream&>(out), L);',
 'parenthesized': '(out << L);',
 'void-return': 'return (void)(out << L);',
 'expr-temp': 'std::ostream &ret=(out << L); (void)ret;',
 'reference-literal': 'const char *const &str=L; out << str;',
 'local-opaque': 'const char *text=L; std::operator<<(out, text);',
 'if-out': 'if (out) out << L;',
}
flags_sets={'default': ['-O2'], 'rename': ['-O2','-frename-registers'], 'no-sched2':['-O2','-fno-schedule-insns2'], 'sched1':['-O2','-fschedule-insns'], 'no-reorder':['-O2','-fno-reorder-blocks']}
for name,body in forms.items():
 path=O/(name+'.cpp');path.write_text('#include <fstream>\nvoid AsmOutput_WritePrologue(std::ofstream &out) {\n'+body.replace('L',literal)+'\n}\n')
 for cc in ('jan25',):
  for fn,flags in flags_sets.items():
   if fn not in ('default','rename'):continue
   obj=O/(name+'-'+cc+'-'+fn+'.o');p=subprocess.run(['/tmp/vuc-binutils215/g++-'+cc+'-as215',*flags,'-c',str(path),'-o',str(obj)],capture_output=True,text=True)
   if p.returncode:print('ERROR',name,cc,fn,p.stderr[-100:],flush=True);continue
   c=json.loads((R/'config/asm_output_jan25_probe.json').read_text());c['source']='vuc/probes/asm_output_hypothesis.cpp';cfg=O/(name+'.json');cfg.write_text(json.dumps(c))
   q=subprocess.run(['python3',str(R/'scripts/match_probe.py'),'--object',str(obj),'--config',str(cfg)],capture_output=True,text=True)
   if q.returncode:print('REJECT',name,cc,fn,q.stderr[-90:],flush=True);continue
   d=json.loads(q.stdout);print('RESULT',name,cc,fn,d['length'],d['exact_bytes'],d.get('different_offsets',[])[:12],flush=True)
   if d['exact']:
    (O/'EXACT.txt').write_text(name+' '+cc+' '+fn+'\n'); print('MATCHED_BYTE_EXACT',name,cc,fn,flush=True)
