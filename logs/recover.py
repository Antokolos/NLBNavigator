from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json
import os
import subprocess
import sys

work = Path(__file__).resolve().parent
repo = Path('/workspace/NLBNavigator')
catalog = json.loads((repo / 'tests/differential/catalog.json').read_text())['routes']
roots = [work / 'results', *sorted(work.glob('retries*'))]
target = work / sys.argv[1]
target.mkdir()

def good(folder, name):
    path = folder / (name + '.exit')
    return path.is_file() and path.read_text().strip() == '0'

jobs = []
for row in catalog:
    key = row['id']
    if not any(all(good(root / key, x) for x in ['nlb', 'instead', 'compare']) for root in roots):
        kind = 'retry-instead' if good(work / 'results' / key, 'nlb') else 'pair'
        jobs.append((kind, key, str(600 + len(jobs)), row['path']))

def run(args):
    subprocess.run(['bash', str(work / 'run-recovery.sh'), *args], cwd=repo,
                   env={**os.environ, 'NLB_RESULT_ROOT': str(target)})

print('Повторные прогоны:', len(jobs), flush=True)
with ThreadPoolExecutor(max_workers=2) as executor:
    list(executor.map(run, jobs))
