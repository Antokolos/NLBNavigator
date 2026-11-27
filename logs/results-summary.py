from pathlib import Path
import hashlib
import json
import re

work = Path(__file__).resolve().parent
repo = Path('/workspace/NLBNavigator')
head = (work / 'tested-sha').read_text().strip()
runner_sha = hashlib.sha256((repo / 'tests/differential/run_route.py').read_bytes()).hexdigest()

def read(path):
    return json.loads(path.read_text(encoding='utf-8'))

def status(dir_path, label):
    return int((dir_path / (label + '.exit')).read_text())

def paired_codes(dir_path):
    keys = ('nlb', 'instead', 'compare')
    if all((dir_path / (key + '.exit')).is_file() for key in keys):
        return [status(dir_path, key) for key in keys]
    return None

def successful(path):
    data = read(path)
    assert data['ok'] and not data['errors'], path
    return data

roots = [work / 'results', *sorted(work.glob('retries*'))]
rows = []
for entry in read(repo / 'tests/differential/catalog.json')['routes']:
    name = entry['id']
    attempts = {root.name: paired_codes(root / name) for root in roots if (root / name).exists()}
    last = next((root / name for root in reversed(roots) if attempts.get(root.name) == [0, 0, 0]), None)
    assert last, (name, attempts)
    compare = successful(last / 'comparison-report.json')
    navigator = successful(last / 'nlbnavigator' / 'route-report.json')
    instead = successful(last / 'instead' / 'route-report.json')
    assert navigator['metadata']['navigator_commit'] == head, name
    assert instead['process']['exit_code'] == 0, name
    assert instead['metadata']['runner_sha256'] == navigator['metadata']['runner_sha256'] == runner_sha, name
    nlb_home = last / 'nlbnavigator' / 'nlbnavigator-home'
    instead_home = last / 'instead' / 'instead-home'
    appdata = Path(instead['metadata']['appdata'])
    for folder in (nlb_home, instead_home, appdata):
        assert folder.is_dir() and folder.resolve() == folder and folder.is_relative_to(last), folder
    rows.append({'route': name, 'checkpoints': len(compare['steps']),
                 'text_pairs': len(compare['text_comparison']),
                 'advance_pairs': len(compare['advance_comparison']),
                 'codes': [0, 0, 0], 'attempts': attempts,
                 'directory': str(last.relative_to(work))})

c6_dir = work / 'results' / 'c6-final' / 'scenarios'
c6 = {}
for label, exit_code in {'cycles': 0, 'restart-save': 0, 'restart-load': 0, 'missing-slot': 1}.items():
    data = read(c6_dir / label / 'trace-proof.json')
    assert data['runner_exit'] == exit_code and data['process']['exit_code'] == 0
    assert data['headers'] == 1
    c6_report = read(c6_dir / label / 'route-report.json')
    assert c6_report['metadata']['runner_sha256'] == runner_sha
    appdata = Path(c6_report['metadata']['appdata'])
    assert appdata.is_dir() and appdata.resolve() == appdata and appdata.parent == c6_dir / label
    c6[label] = dict(runner_exit=exit_code, process=data['process'], first_seq=data['first_seq'],
                     last_seq=data['last_seq'], commands=[
                         dict(seq=event['command']['seq'], command=event['command']['command'],
                              kind=event['command']['kind'], rendered=event['command']['rendered'])
                         for event in data['commands']])
slots = read(c6_dir / 'slots.json')
assert slots['original']['sha256'] == slots['copied']['sha256']
achievement = {}
for label in ('award', 'readback', 'clean', 'reaward'):
    report = successful(work / 'results' / 'achievement' / 'scenarios' / label / 'route-report.json')
    assert report['metadata']['navigator_commit'] == head
    achievement[label] = len(report['steps'])
for label in ('save_load', 'save_portion', 'achievement'):
    assert status(work / 'results' / label, 'nlb') == 0
assert status(work / 'results' / 'c6-final', 'instead') == 0
for label in ('configure', 'build', 'ctest', 'catalog', 'python', 'diff-check', 'c8-after'):
    assert status(work, label) == 0
assert status(work, 'c8-before') == 1
c5 = {path.stem: int(path.read_text()) for path in sorted((work / 'c5').glob('*.exit'))}
assert len(c5) == 11 and set(c5.values()) == {0}
before = (work / 'c8-before.log').read_text()
after = (work / 'c8-after.log').read_text()
python_log = (work / 'python.log').read_text()
python_tests = int(re.search(r'Ran (\d+) tests', python_log)[1])
python_skips = int(re.search(r'skipped=(\d+)', python_log)[1])
results = {
    'routes': len(rows),
    'runner_sha256': runner_sha,
    'checkpoints': sum(row['checkpoints'] for row in rows),
    'text_pairs': sum(row['text_pairs'] for row in rows),
    'advance_pairs': sum(row['advance_pairs'] for row in rows),
    'python': {'passed': python_tests - python_skips, 'skipped': python_skips, 'failed': 0},
    'c8': {'before_failures': int(re.search(r'failures=(\d+)', before)[1]),
           'after_tests': int(re.search(r'Ran (\d+) tests', after)[1])},
    'c5': c5, 'c6': c6, 'slots': slots, 'achievement': achievement,
}
assert results['routes'] == 22
(work / 'routes.json').write_text(json.dumps(rows, indent=2, ensure_ascii=False) + '\n')
(work / 'verified-c7.json').write_text(json.dumps(results, indent=2, ensure_ascii=False) + '\n')
for row in rows:
    print(f"| [{row['route']}]({row['directory']}/comparison-report.json) | {row['checkpoints']} | 0 / 0 / 0 |")
