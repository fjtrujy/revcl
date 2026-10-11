#!/usr/bin/env python3
"""Test historical GCC epilogue decisions without hand-written instructions."""
import importlib.util
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TOOL = Path('/tmp/vuc-binutils215')
spec = importlib.util.spec_from_file_location('matcher', ROOT / 'vuc/scripts/match_probe.py')
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
source = ROOT / 'vuc/probes/instruction_arena_cleanup.cpp'
config = ROOT / 'vuc/config/instruction_arena_cleanup_probe.json'
options = {
    'baseline': [],
    'no-defer-pop': ['-fno-defer-pop'],
    'no-force-mem': ['-fno-force-mem'],
    'no-function-cse': ['-fno-function-cse'],
    'no-schedule-insns': ['-fno-schedule-insns'],
    'no-guess-branch': ['-fno-guess-branch-probability'],
    'no-branch-count': ['-fno-branch-count-reg'],
    'no-strength-reduce': ['-fno-strength-reduce'],
    'no-caller-saves': ['-fno-caller-saves'],
    'no-peephole': ['-fno-peephole'],
    'no-crossjump': ['-fno-crossjumping'],
    'no-reorder-blocks': ['-fno-reorder-blocks'],
    'no-if-conversion2': ['-fno-if-conversion2'],
    'no-expensive': ['-fno-expensive-optimizations'],
    'no-rename': ['-fno-rename-registers'],
    'stack-align-4': ['-mpreferred-stack-boundary=2'],
    'stack-align-8': ['-mpreferred-stack-boundary=3'],
    'omit-frame': ['-fomit-frame-pointer'],
    'no-omit-frame': ['-fno-omit-frame-pointer'],
    'schedule-insns': ['-fschedule-insns'],
    'inline-functions': ['-finline-functions'],
    'no-builtin': ['-fno-builtin'],
}
for label, extra in options.items():
    obj = TOOL / ('cleanup-epilogue-' + label + '.o')
    args = [str(TOOL / 'g++-jan25-as215'), '-O2', '-frename-registers',
            '-fuse-cxa-atexit', *extra, '-c', str(source), '-o', str(obj)]
    cp = subprocess.run(args, capture_output=True, text=True)
    if cp.returncode:
        print(label, 'compile error', cp.stderr.strip()[-120:], flush=True)
        continue
    try:
        result = matcher.run(obj, config)
        print(label, result['exact_bytes'], '/', result['length'],
              'EXACT' if result['exact'] else '', flush=True)
    except ValueError as exc:
        print(label, str(exc), flush=True)
