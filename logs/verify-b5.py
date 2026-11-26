from pathlib import Path
import json

work = Path(__file__).resolve().parent
root = Path('/workspace/NLBNavigator')
routes = json.loads((root / 'tests/differential/catalog.json').read_text())['routes']
roots = [work / 'results'] + sorted(work.glob('retries*'))
head = (work / 'tested-sha').read_text().strip()
book = '888b4b7a7a564693274f53341728ee9f3b4c2879'
export = '0db8d2c611faa596987baaf041763ef6c9282484'
theme = '1a07e0ffc0a1e8402b9d47a104d88d3f33204a7e'

def good(folder):
    return all((folder / (key + '.exit')).is_file() and (folder / (key + '.exit')).read_text().strip() == '0'
               for key in ('nlb', 'instead', 'compare'))

def report(path):
    with path.open() as file:
        data = json.load(file)
    assert data['ok'] and not data['errors'], path
    return data

count = {'routes': 0, 'checkpoints': 0, 'text_pairs': 0, 'advance_pairs': 0}
for row in routes:
    key = row['id']
    folder = next(base / key for base in reversed(roots) if good(base / key))
    navigator = report(folder / 'nlbnavigator/route-report.json')
    instead = report(folder / 'instead/route-report.json')
    compare = report(folder / 'comparison-report.json')
    assert navigator['metadata']['navigator_commit'] == head, key
    assert navigator['metadata']['book_commit'] == book, key
    assert navigator['metadata']['theme_commit'] == instead['metadata']['instead_export_commit'] == export, key
    assert navigator['metadata']['base_theme_commit'] == instead['metadata']['base_theme_commit'] == theme, key
    steps = len(compare['steps'])
    assert steps == len(navigator['steps']) == len(instead['steps']), key
    assert all(x['width'] == 1920 and x['height'] == 1080 for x in navigator['steps'] + instead['steps']), key
    for item in compare['text_comparison']:
        for runtime in ('instead', 'nlbnavigator'):
            if item[runtime + '_expected_fragment']:
                assert item[runtime + '_check'] and item[runtime + '_check']['passed'], key
    assert all(not x['mismatch'] for x in compare['advance_comparison']), key
    count['routes'] += 1
    count['checkpoints'] += steps
    count['text_pairs'] += len(compare['text_comparison'])
    count['advance_pairs'] += len(compare['advance_comparison'])
print(json.dumps(count, indent=2))
