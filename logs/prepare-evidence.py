from pathlib import Path
import hashlib
import json
import subprocess

work = Path(__file__).resolve().parent
repo = Path('/workspace/NLBNavigator')
evidence = work / 'evidence'
evidence.mkdir()

meta = {
    'repository': 'Antokolos/NLBNavigator', 'draft_pr': 73, 'issue': 69,
    'initial_head': '5ecbe9ae8ff75dfd3fbdbcc9a3dee697b22fa413',
    'merge_head': (work / 'merge-sha').read_text().strip(),
    'tested_head': (work / 'tested-sha').read_text().strip(),
    'main': subprocess.check_output(['git', '-C', str(repo), 'rev-parse', 'origin/main'], text=True).strip(),
    'pins': dict(line.split() for line in (work / 'pins-before').read_text().splitlines() if len(line.split()) == 2),
    'environment': {'os': 'Debian 13.7 trixie amd64', 'display': 'Xvfb 1920x1080x24 -noreset',
                    'sdl_video': 'x11', 'sdl_audio': 'dummy', 'magick_threads': 2, 'omp_threads': 2},
    'tools': {'cmake': '3.31.6', 'ninja': '1.12.1', 'gcc': '14.2.0', 'python': '3.12.14',
              'pillow': '13.0.0', 'instead': '3.5.2', 'luajit': '2.1.0+openresty20250117-2',
              'xvfb': '2:21.1.16-1.3+deb13u4', 'openbox': '3.6.1-12+b2',
              'xdotool': '1:3.20160805.1-5.1', 'imagemagick': '7.1.1-43',
              'sdl3': '3.4.18', 'sdl3_image': '3.2.4', 'sdl3_ttf': '3.2.2', 'sdl3_mixer': '3.2.4'},
    'results': json.loads((work / 'verified-c7.json').read_text()),
}
(evidence / 'metadata.json').write_text(json.dumps(meta, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')

def copy_file(src, target):
    target.parent.mkdir(parents=True, exist_ok=True)
    data = src.read_bytes()
    if len(data) <= 900_000:
        target.write_bytes(data)
        return
    part = bytearray()
    number = 1
    for line in data.splitlines(keepends=True):
        if part and len(part) + len(line) > 900_000:
            target.with_name(f'{target.name}.part-{number:03d}').write_bytes(part)
            number += 1
            part.clear()
        part.extend(line)
    if part:
        target.with_name(f'{target.name}.part-{number:03d}').write_bytes(part)

for filename in ('synchronization.log', 'pins-before', 'pins-after', 'paths.log', 'versions.log',
                 'checkouts.log', 'stead-serialized-state.log', 'configure.log', 'build.log', 'build.exit', 'build-final.log',
                 'build-final.exit', 'ctest.log', 'ctest.exit', 'python.log', 'python.exit',
                 'python-early.log', 'python-early.exit', 'catalog.log', 'catalog.exit', 'c6-units.log',
                 'c6-units.exit', 'local-suite.log', 'summary.md', 'verified-c7.json', 'commands.txt',
                 'run-final.sh', 'local-suite.py', 'lua-run.c', 'capture-tools/import',
                 'slow-input/xdotool', 'wm-tools/openbox', 'prepare-evidence.py'):
    path = work / filename
    if path.is_file():
        copy_file(path, evidence / 'logs' / filename)

suffixes = {'.tsv', '.jsonl', '.json', '.log', '.exit', '.command', '.script', '.txt', '.xdpyinfo', '.sav'}
roots = ('results', 'c5-before', 'c5-final', 'restart-debug', 'restart-old')
for root in [*(work / name for name in roots), *sorted(work.glob('retries*'))]:
    for src in root.rglob('*'):
        if src.is_symlink() or not src.is_file():
            continue
        rel = src.relative_to(root)
        if set(rel.parts).intersection({'home', 'tmp', 'instead-home', 'nlbnavigator-home',
                                        'instead-appdata', 'instead-game-overlay', 'diff'}):
            if (src.name == 'save9' and rel.parts[0] == 'c6-final'
                    or src.name == 'slot1.sav' and rel.parts[0] in {'save_load', 'save_portion'}):
                copy_file(src, evidence / 'saves' / rel)
            continue
        if src.suffix in suffixes:
            copy_file(src, evidence / root.name / rel)
