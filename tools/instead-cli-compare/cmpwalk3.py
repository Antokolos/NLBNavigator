#!/usr/bin/env python3
# cmpwalk3.py <каталог игры INSTEAD> <книга> <сценарий> <переменные...>
# Сценарий по подписям проигрывается в NLBNavigator (drive с трассировкой) и в INSTEAD (instead-cli
# со служебными командами nlbnoop-номерами, /actid, /invid, /useid, /var); после каждого шага
# сравниваются указанные переменные (объекты — по имени).
# Пути: NLB_INSTEAD_CLI (каталог release с instead-cli), NLB_DRIVE (отладочный драйвер плеера).
import os, re, subprocess, sys
GAME, BOOK, SCRIPT, VARS = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4:]
CLI = os.environ.get('NLB_INSTEAD_CLI', '/tmp/ic/instead-cli/release')
DRIVE = os.environ.get('NLB_DRIVE', '/tmp/fr/d/drive')
env = dict(os.environ, NLBNAV_TRACE='1', NLBNAV_VARS=' '.join(VARS))
trace = subprocess.run([DRIVE, BOOK, SCRIPT, '1'], capture_output=True, env=env).stdout.decode('utf-8', 'replace')
steps, pending = [], None
for line in trace.split('\n'):
    if line.startswith('TRACE '): pending = line[6:].split()
    elif line.startswith('>>> '):
        steps.append({'cmd': line[4:].replace('   [НЕ ВЫПОЛНЕНО]', ''), 'trace': pending, 'bad': 'НЕ ВЫПОЛНЕНО' in line}); pending = None
    elif line.startswith('STATE ') and steps: steps[-1]['state_raw'] = line[6:].split(' page=')[0]
def run(cmds):
    r = subprocess.run(['./instead-cli', '-e', '-w1000', GAME], cwd=CLI, input=('\n'.join(cmds + ['/quit']) + '\n').encode(),
                       capture_output=True, timeout=300)
    return r.stdout.decode('utf-8', 'replace')
def norm(v):
    v = {'nil': 'false'}.get(v, v)
    return re.sub(r'\.0$', '', v)
icmds = []; bad = 0
for i, st in enumerate(steps):
    c, tr = st['cmd'], st['trace']
    if c.startswith('c:'):
        text = c[2:]; found = False
        for probe in ('nlbnoop', '/look'):
            screen = run(icmds + [probe]).split('\n> ')[-2]
            m = re.findall(re.escape(text) + r'\((\d+)\)', screen.replace('\n', ' '))
            if m: icmds += [probe, m[0]]; found = True; break
        if not found: print(f'шаг {i+1}: в INSTEAD нет ссылки «{text}»'); bad += 1; break
    elif c == 'l': icmds.append('/look')
    elif tr and tr[0] == 'act': icmds.append('/actid ' + tr[1])
    elif tr and tr[0] == 'inv': icmds.append('/invid ' + tr[1])
    elif tr and tr[0] == 'use': icmds.append('/useid ' + tr[1] + ' ' + tr[2])
    out = run(icmds + ['/var ' + ' '.join(VARS)])
    line = re.findall(r'NLBVARS (.*)', out)[-1] if 'NLBVARS' in out else ''
    got = dict(re.findall(r'(\w+)=(.*?)(?= \w+=|$)', line))  # значения могут содержать пробелы
    ins = [norm(got.get(v, '?')) for v in VARS]
    # значения плеера разделены пробелами, но строки сами могут их содержать: сопоставляем с
    # числом значений INSTEAD, склеивая хвост строковых значений
    raw = st.get('state_raw', '').split(' ')
    ours = []
    for k, v in enumerate(VARS):
        want = len(got.get(v, 'x').split(' '))
        ours.append(norm(' '.join(raw[:want]))); raw = raw[want:]
    diff = [f'{v}: INSTEAD {a} / плеер {b}' for v, a, b in zip(VARS, ins, ours) if a != b]
    print(f'{i+1:>2} {c[:30]:30} ' + ('совпадает' if not diff else 'РАСХОЖДЕНИЕ: ' + '; '.join(diff)))
    bad += bool(diff)
print(f'шагов: {len(steps)}, с расхождениями: {bad}')
