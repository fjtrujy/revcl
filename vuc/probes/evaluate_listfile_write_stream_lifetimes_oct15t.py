#!/usr/bin/env python3
"""Genuine C++03 ListFile::WriteToFile stream/line iteration alternatives.

Original 1071-byte ELF calls ofstream filebuf.open, prints signed
line index, literal ':\\t', string c_str(), std::endl and destroys
ofstream. Only original-length, all-58-relocation, all-3-literal-byte
candidates are eligible for experimental selection.
"""
from pathlib import Path
import importlib.util,subprocess,json
R=Path(__file__).resolve().parents[1]
OUT=Path('/tmp/vuc-binutils215/listfile-stream-lifetimes-oct15t')
OUT.mkdir(exist_ok=True)
src=(R/'probes/ListFileWriteToFileFreshOct15t.cpp').read_text()
span='''    for (int i=0;i<n;++i) {
        output << i << ":\\t" << lines_[i].c_str() << std::endl;
    }
'''
assert span in src
forms={
 'selected':src,
 'const-reference':src.replace(span,'''    for (int i=0;i<n;++i) {
        const std::string &line=lines_[i];
        output << i << ":\\t" << line.c_str() << std::endl;
    }
'''),
 'reference':src.replace(span,'''    for (int i=0;i<n;++i) {
        std::string &line=lines_[i];
        output << i << ":\\t" << line.c_str() << std::endl;
    }
'''),
 'char-local':src.replace(span,'''    for (int i=0;i<n;++i) {
        const char *line=lines_[i].c_str();
        output << i << ":\\t" << line << std::endl;
    }
'''),
 'pointer-local':src.replace(span,'''    for (int i=0;i<n;++i) {
        const std::string *line=&lines_[i];
        output << i << ":\\t" << line->c_str() << std::endl;
    }
'''),
 'output-in-two':src.replace('output << i << ":\\t" << lines_[i].c_str() << std::endl;', 'output << i << ":\\t"; output << lines_[i].c_str() << std::endl;'),
 'output-in-three':src.replace('output << i << ":\\t" << lines_[i].c_str() << std::endl;', 'output << i << ":\\t"; output << lines_[i].c_str(); output << std::endl;'),
 'output-newline-separated':src.replace('output << i << ":\\t" << lines_[i].c_str() << std::endl;', 'output << i << ":\\t" << lines_[i].c_str(); output << std::endl;'),
 'explicit-output-mode':src.replace('std::ofstream output(path);','std::ofstream output(path,std::ios::out);'),
 'if-fail':src.replace('if (!output) return false;', 'if (output.fail()) return false;'),
 'if-fail-else':src.replace('if (!output) return false;', 'if (output.fail()) return false; else {}'),
 'if-stream-else':src.replace('if (!output) return false;', 'if (output) {} else return false;'),
 'pointer-index':src.replace('lines_[i].c_str()', '(*(lines_.begin()+i)).c_str()'),
 'while-index':src.replace(span,'''    int i=0;
    while(i<n) {
        output << i << ":\\t" << lines_[i].c_str() << std::endl;
        ++i;
    }
'''),
}
sp=importlib.util.spec_from_file_location('m',R/'scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=R/'config/listfile_write_to_file_partial_oct15t.json'
results=[]
for label,code in forms.items():
 cpp=OUT/(label+'.cpp');cpp.write_text(code)
 for snap in ('jan25','dec06'):
  obj=OUT/(label+'-'+snap+'.o')
  p=subprocess.run(['/tmp/vuc-binutils215/g++-'+snap+'-as215','-O2','-frename-registers','-falign-jumps=4','-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
  row={'source':label,'compiler':snap}
  if p.returncode:row['error']=p.stderr[-140:]
  else:
   try:
    v=m.run(obj,cfg)
    row.update(length=v['length'],matching=v['exact_bytes'],relocations=v['relocations'],literals=v['verified_literal_bytes'])
   except ValueError as e:row['error']=str(e)
  results.append(row);print(row,flush=True)
(OUT/'results.json').write_text(json.dumps(results,indent=2)+'\n')
