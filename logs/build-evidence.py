from pathlib import Path
import hashlib
import json
import re
import shutil
import stat

work = Path(__file__).resolve().parent
evidence = work / 'evidence'
evidence.mkdir()

def copy(src, dest):
    resolved = src.resolve(strict=True)
    assert src == resolved and src.is_relative_to(work), src
    assert stat.S_ISREG(src.stat().st_mode), src
    dest.parent.mkdir(parents=True, exist_ok=True)
    if src.stat().st_size < 900_000:
        shutil.copyfile(src, dest)
        return
    assert src.suffix != '.sav' and src.name != 'save9', src
    part_number = 1
    part = bytearray()
    with src.open('rb') as data:
        for line in data:
            assert len(line) < 900_000, src
            if len(part) + len(line) >= 900_000:
                dest.with_name(f'{dest.name}.part-{part_number:03d}').write_bytes(part)
                part_number += 1
                part.clear()
            part.extend(line)
    if part:
        dest.with_name(f'{dest.name}.part-{part_number:03d}').write_bytes(part)

logs = (
    'build-evidence.py', 'verify-evidence-api.py', 'finalize-local.sh', 'check-pins.sh', 'commands.txt', 'compile.log', 'capture-tools/import',
    'wm-tools/openbox', 'slow-input/xdotool', 'lua-run.c', 'openbox.xml', 'routes.json',
    'run-final.sh', 'local-suite.py', 'verify-c7.py', 'results-summary.py', 'c8-tests.diff',
    'versions.log', 'versions-runtime.log', 'versions-packages.log', 'paths.log', 'pins-before', 'pins-after',
    'refs-before', 'tested-sha', 'worktree-before.log', 'worktree-after.log',
)
for name in logs + tuple(f'{prefix}.{ext}' for prefix in
                         ('configure', 'build', 'ctest', 'c5-compile', 'c8-before', 'c8-after',
                          'python', 'catalog', 'diff-check', 'graphical-suite', 'verified-c7')
                         for ext in ('log', 'exit', 'command', 'json')) + ('c6.command',):
    src = work / name
    if src.is_file():
        copy(src, evidence / 'logs' / name)

text_suffixes = {'.tsv', '.jsonl', '.json', '.log', '.exit', '.command', '.script', '.txt', '.xdpyinfo'}
profile_dirs = {'home', 'tmp', 'instead-home', 'nlbnavigator-home', 'instead-appdata', 'instead-game-overlay'}
roots = [work / 'c5', work / 'results', *sorted(work.glob('retries*'))]
for root in roots:
    for src in sorted(root.rglob('*')):
        if src.is_symlink() or not src.is_file():
            continue
        rel = src.relative_to(root)
        if set(rel.parts).intersection(profile_dirs):
            if (src.name == 'save9' and rel.parts[0] == 'c6-final'
                    or src.name == 'slot1.sav' and rel.parts[0] in {'save_load', 'save_portion'}):
                copy(src, evidence / 'saves' / root.name / rel)
            continue
        if src.suffix in text_suffixes:
            copy(src, evidence / root.name / rel)

meta = {
    'repository': 'Antokolos/NLBNavigator',
    'draft_pr': 73,
    'issue': 69,
    'review': 'C8',
    'initial_head': 'b104fddf6c9d10f3ae7df6ee5e5b7a9674a6b815',
    'tested_head': (work / 'tested-sha').read_text().strip(),
    'main': 'c90dfb725fb3a5ec7093078634ec0029f5582423',
    'ue': '67f7de82bf8007c680908d98d0c8033517a52cde',
    'prior_evidence': '539331bbcad7b654a9002c4aacba3c91a978bde4',
    'pins': dict(line.split() for line in (work / 'pins-before').read_text().splitlines()),
    'environment': {
        'os': 'Debian 13.7 trixie amd64',
        'display': 'Xvfb 1920x1080x24 -noreset',
        'sdl_video': 'x11', 'sdl_audio': 'dummy',
        'magick_threads': 2, 'omp_threads': 2, 'llvmpipe_threads': 2,
        'xdotool_click_pause_seconds': 0.27,
        'graphical_workers': 3,
    },
    'tools': {
        'cmake': '3.31.6', 'ninja': '1.12.1', 'gcc': '14.2.0', 'python': '3.12.14',
        'pillow': '13.0.0', 'instead': '3.5.2',
        'luajit': '2.1.0+openresty20250117-2', 'xvfb': '2:21.1.16-1.3+deb13u4', 'openbox': '3.6.1', 'xdotool': '3.20160805.1',
        'imagemagick': '7.1.1-43', 'sdl3': '3.4.18', 'sdl3_image': '3.2.4',
        'sdl3_ttf': '3.2.2', 'sdl3_mixer': '3.2.4',
    },
    'results': json.loads((work / 'verified-c7.json').read_text()),
}
(evidence / 'metadata.json').write_text(json.dumps(meta, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
copy(work / 'REVIEW_REPORT.md', evidence / 'REVIEW_REPORT.md')

secret = re.compile(rb'github_pat_[a-zA-Z0-9_]{20,}|gh[pousr]_[a-zA-Z0-9]{30,}|'
                    rb'AKIA[A-Z0-9]{16}|-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----')
sums = []
for path in sorted(evidence.rglob('*')):
    if not path.is_file():
        continue
    contents = path.read_bytes()
    assert not secret.search(contents), path
    sums.append(f'{hashlib.sha256(contents).hexdigest()}  {path.relative_to(evidence)}')
(evidence / 'SHA256SUMS.txt').write_text('\n'.join(sums) + '\n', encoding='utf-8')
print(f'Проверено {len(sums)} файлов')
