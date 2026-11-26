from pathlib import Path
import json

work = Path(__file__).resolve().parent
catalog = Path('/workspace/NLBNavigator/tests/differential/catalog.json')
routes = json.loads(catalog.read_text())['routes']
roots = [work / 'results'] + sorted(work.glob('retries*'))

def exit_code(folder, key):
    path = folder / (key + '.exit')
    return path.read_text().strip() if path.is_file() else '-'

print('| Маршрут | NLBNavigator | INSTEAD | Compare | Контрольных точек | Каталог |')
print('|---|---:|---:|---:|---:|---|')
bad = []
for item in routes:
    name = item['id']
    runtime = ('nlb', 'instead', 'compare')
    candidates = [(root / name, [exit_code(root / name, key) for key in runtime]) for root in roots]
    folder, codes = next(((folder, codes) for folder, codes in reversed(candidates) if all(code == '0' for code in codes)), candidates[0])
    path = folder / 'comparison-report.json'
    report = json.loads(path.read_text()) if path.is_file() else {}
    count = len(report.get('steps', [])) if report.get('ok') else 0
    print('|', name, '|', ' | '.join(codes), '|', count, '|', folder.parent.name, '|')
    if codes != ['0', '0', '0'] or not report.get('ok') or report.get('errors'):
        bad.append(name)
for name in ('save_load', 'save_portion', 'achievement'):
    choices=[(root / name, exit_code(root / name, 'nlb')) for root in roots]
    folder, code = next(((folder, code) for folder, code in reversed(choices) if code == '0'), choices[0])
    print(name, code, folder.parent.name)
    if code != '0':
        bad.append(name)
print('Ожидают успешного результата:', ', '.join(bad) or 'нет')
raise SystemExit(bool(bad))
