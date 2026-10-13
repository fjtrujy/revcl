#!/usr/bin/env python3
"""GCC 3.3.3 C++ receiver-lifetime hypotheses from VCL link_oParts evidence.

These source changes preserve real AST printing behavior and never inject code.
Probe results remain experimental until all 98 original bytes and relocations match.
"""
import importlib.util
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRATCH = Path('/tmp/vuc-binutils215/printer-receiver-oct13')
SCRATCH.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location('matcher', ROOT / 'vuc/scripts/match_probe.py')
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
source = (ROOT / 'vuc/src/Printer.cpp').read_text()
source = source.replace('"../src/ASTNodeList.h"', '"' + str(ROOT / 'vuc/src/ASTNodeList.h') + '"')
old = '''    Print(call->callee, output);
    output << "(";
    Print(call->arguments, output, ", ");
    output << ")";'''
assert old in source
variants = {
    'original': old,
    'callee-alias': '''    ASTLValExpr *callee = call->callee;
    Print(callee, output);
    output << "(";
    Print(call->arguments, output, ", ");
    output << ")";''',
    'arguments-alias': '''    Print(call->callee, output);
    output << "(";
    ASTNodeList<ASTExpr *> &args = call->arguments;
    Print(args, output, ", ");
    output << ")";''',
    'both-alias': '''    ASTLValExpr *callee = call->callee;
    ASTNodeList<ASTExpr *> &args = call->arguments;
    Print(callee, output);
    output << "(";
    Print(args, output, ", ");
    output << ")";''',
    'stream-alias': '''    std::ostream &stream = output;
    Print(call->callee, stream);
    stream << "(";
    Print(call->arguments, stream, ", ");
    stream << ")";''',
    'stream-pointer': '''    std::ostream *stream = &output;
    Print(call->callee, *stream);
    *stream << "(";
    Print(call->arguments, *stream, ", ");
    *stream << ")";''',
    'callee-reference': '''    ASTFunCall &fun = *call;
    Print(fun.callee, output);
    output << "(";
    Print(fun.arguments, output, ", ");
    output << ")";''',
    'scoped-arguments': '''    Print(call->callee, output);
    output << "(";
    { ASTNodeList<ASTExpr *> &args = call->arguments;
      Print(args, output, ", "); }
    output << ")";''',
    'literal-locals': '''    const char *open = "(";
    const char *between = ", ";
    const char *close = ")";
    Print(call->callee, output);
    output << open;
    Print(call->arguments, output, between);
    output << close;''',
}
configs = [('plain', ['-O2']), ('rename', ['-O2', '-frename-registers'])]
for label, body in variants.items():
    cpp = SCRATCH / (label + '.cpp')
    cpp.write_text(source.replace(old, body))
    for flags_name, flags in configs:
        obj = SCRATCH / (label + '-' + flags_name + '.o')
        args = ['/tmp/vuc-binutils215/g++-jan25-as215', *flags,
                '-fuse-cxa-atexit', '-ffunction-sections', '-c', str(cpp), '-o', str(obj)]
        compiled = subprocess.run(args, capture_output=True, text=True)
        if compiled.returncode:
            print(label, flags_name, 'ERROR', compiled.stderr[-140:], flush=True)
            continue
        try:
            result = matcher.run(obj, ROOT / 'vuc/config/print_ast_fun_context_oct14_probe.json')
            print(label, flags_name, result['exact_bytes'], result['length'],
                  result['different_offsets'], flush=True)
            if result['exact']:
                print('BYTE-EXACT', cpp, flush=True)
                raise SystemExit(0)
        except ValueError as exc:
            print(label, flags_name, 'nonmatching length or relocation', str(exc)[:100], flush=True)
