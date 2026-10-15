#!/usr/bin/env python3
"""Probe ListFile::WriteToFile in original method-order C++03 unit."""
from pathlib import Path
import subprocess,json
R=Path(__file__).resolve().parents[1];O=Path('/tmp/listfile-real-owner-oct15u');O.mkdir(exist_ok=True)
unit=(R/'src/ListFileUnit.cpp').read_text();p=(R/'probes/ListFileWriteToFileFreshOct15t.cpp').read_text();body=p[p.index('bool ListFile::WriteToFile(const char *path)'):]
unit=unit.replace('    void Generate(const char *filename);','    void Generate(const char *filename);\n    bool WriteToFile(const char *path);')
for label,source in [('before-getnumlines',unit.replace('int ListFile::GetNumLines() {',body+'\nint ListFile::GetNumLines() {')),('after-getnumlines',unit+'\n'+body)]:
 path=O/(label+'.cpp');path.write_text(source)
 for compiler in ('jan25','dec06'):
  obj=O/(label+'-'+compiler+'.o');cmd=['/tmp/vuc-binutils215/g++-'+compiler+'-as215','-O2','-frename-registers','-falign-jumps=4','-c',str(path),'-o',str(obj)];x=subprocess.run(cmd,capture_output=True,text=True)
  if x.returncode:print(label,compiler,x.stderr[-280:],flush=True);continue
  cfg=json.loads((R/'config/listfile_write_to_file_partial_oct15t.json').read_text());c=O/(label+'.json');c.write_text(json.dumps(cfg));q=subprocess.run(['python3',str(R/'scripts/match_probe.py'),'--object',str(obj),'--config',str(c)],capture_output=True,text=True)
  if q.returncode:print(label,compiler,q.stderr[-280:],flush=True)
  else:
   v=json.loads(q.stdout);print(label,compiler,'length',v['length'],'matching',v['exact_bytes'],'relocations',v['relocations'],'literals',v['verified_literal_bytes'],flush=True)
