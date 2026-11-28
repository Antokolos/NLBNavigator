from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json
import os
import subprocess
import sys

work = Path(__file__).resolve().parent
repo = Path('/workspace/nlb-issue76.nHxaHSiy/repo')
routes = json.loads((repo / 'tests/differential/catalog.json').read_text())['routes']
assert len(routes) == 22 and all(row['book_id'] == 'wolfman' for row in routes)
jobs = [
    ('check', 'save_load', '701'),
    ('check', 'save_portion', '702'),
    ('check', 'achievement', '703', '--case-timeout', '800'),
]
jobs += [('pair', row['id'], str(710 + idx), row['path']) for idx, row in enumerate(routes)]

def run(args):
    env = dict(os.environ, NLB_CLICK_PAUSE='0.27')
    proc = subprocess.run(['bash', str(work / 'run-final.sh'), *args], cwd=repo, env=env)
    print(f'{args[1]}: код оболочки {proc.returncode}', flush=True)

with ThreadPoolExecutor(max_workers=int(sys.argv[1])) as executor:
    list(executor.map(run, jobs))
