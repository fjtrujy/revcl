#!/usr/bin/env python3
"""Identify which genuine recovered VUC predecessors affect Canonize2 registers."""
import importlib.util
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TMP = Path('/tmp/vuc-binutils215/canonize-context-oct13')
TMP.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location('matcher', ROOT / 'vuc/scripts/match_probe.py')
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
original_config = ROOT / 'vuc/config/canonize2_reduce_probe.json'
parts = {
    'predicate': ROOT / 'vuc/src/IRPredicatesEnvironments.cpp',
    'conversions': ROOT / 'vuc/src/ConvVal.cpp',
    'target': ROOT / 'vuc/probes/canonize2_reduce.cpp',
}
variants = {
    'isolated': ['target'],
    'predicate': ['predicate', 'target'],
    'conversions': ['conversions', 'target'],
    'predicate-conversions': ['predicate', 'conversions', 'target'],
    'conversions-predicate': ['conversions', 'predicate', 'target'],
    'target-predicate-conversions': ['target', 'predicate', 'conversions'],
}
for label, items in variants.items():
    source = TMP / (label + '.cpp')
    source.write_text('\n'.join('#include "' + str(parts[item]) + '"' for item in items) + '\n')
    obj = TMP / (label + '.o')
    compiled = subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',
                               '-O2', '-frename-registers', '-falign-jumps=1',
                               '-c', str(source), '-o', str(obj)],
                              text=True, capture_output=True)
    if compiled.returncode:
        print(label, 'compile error', compiled.stderr[-150:], flush=True)
        continue
    try:
        result = matcher.run(obj, original_config)
        print(label, result['exact_bytes'], result['length'],
              'EXACT' if result['exact'] else 'PARTIAL', flush=True)
    except ValueError as error:
        print(label, 'validation error', error, flush=True)
