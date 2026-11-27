import base64
import json
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parent / 'evidence'
branch, sha = sys.argv[1:]
repo = 'repos/Antokolos/NLBNavigator'

def get(endpoint, *args):
    return json.loads(subprocess.check_output(['gh', 'api', endpoint, *args]))

ref = get(f'{repo}/git/ref/heads/{branch}')
assert ref['object']['sha'] == sha, ref
paths = [
    'REVIEW_REPORT.md', 'metadata.json', 'SHA256SUMS.txt',
    'logs/c8-before.log', 'logs/c8-after.log', 'logs/ctest.log',
    'logs/python.log', 'logs/catalog.log', 'logs/routes.json', 'c5/repeat.log',
    'results/c6-final/scenarios/cycles/instead-state.tsv',
    'results/c6-final/scenarios/restart-load/trace-proof.json',
    'results/c6-final/scenarios/missing-slot/route-report.json',
    'results/c6-final/scenarios/slots.json',
    'results/achievement/scenarios/achievement-summary.json',
    'results/wolfman-opening/comparison-report.json',
]
for path in paths:
    item = get(f'{repo}/contents/{path}', '-X', 'GET', '-f', f'ref={sha}')
    data = base64.b64decode(item['content'])
    assert data == (root / path).read_bytes(), path
    if path == 'metadata.json':
        declared = json.loads(data)['tested_head']
        expected = (root.parent / 'tested-sha').read_text().strip()
        assert declared == expected, declared
    print(f'{path}: {len(data)} байт, совпадает')
