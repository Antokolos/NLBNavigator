import concurrent.futures
import json
import pathlib
import subprocess
import sys

work = pathlib.Path(__file__).resolve().parent
root = pathlib.Path('/workspace/NLBNavigator')
routes = json.loads((root / 'tests/differential/catalog.json').read_text())['routes']
assert len(routes) == 22 and all(row['book_id'] == 'wolfman' for row in routes)
jobs = [('check', key, str(300 + idx)) for idx, key in enumerate(('save_load', 'save_portion', 'achievement'))]
jobs += [('pair', row['id'], str(400 + idx), row['path']) for idx, row in enumerate(routes)]

def run(args):
    subprocess.run(['bash', str(work / 'run-local.sh'), *args], cwd=root)

with concurrent.futures.ThreadPoolExecutor(max_workers=int(sys.argv[1])) as executor:
    list(executor.map(run, jobs))
results = work / 'results'
bad = []
for kind, label, *_ in jobs:
    for runtime in (('nlb', 'instead', 'compare') if kind == 'pair' else ('nlb',)):
        path = results / label / f'{runtime}.exit'
        if not path.is_file() or path.read_text().strip() != '0':
            bad.append(str(path.relative_to(results)))
print('Неуспешные проверки:', bad)
raise SystemExit(bool(bad))
