#!/usr/bin/env python3
# cmpwalk2.py <trace.out> [--step] — сценарий, проигранный в NLBNavigator (drive с NLBNAV_TRACE=1),
# повторяется в настоящем INSTEAD (instead-cli + служебные команды nlbnoop, /actid, /invid, /useid,
# /var); после каждого шага сравниваются жизнь, энергия и бодрость («Слуга Ра»).
# Пути: переменные окружения NLB_INSTEAD_GAME (копия экспорта с instead_main_append.lua)
# и NLB_INSTEAD_CLI (каталог release с instead-cli).
import os, re, subprocess, sys
GAME = os.environ.get('NLB_INSTEAD_GAME', '/tmp/ic/game/ra')
CLI = os.environ.get('NLB_INSTEAD_CLI', '/tmp/ic/instead-cli/release')
steps = []
cur = None
pending = None
for line in open(sys.argv[1], encoding='utf-8', errors='replace'):
    line = line.rstrip('\n')
    if line.startswith('TRACE '):
        pending = line[6:].split()
    elif line.startswith('>>> '):
        cmd = line[4:].replace('   [НЕ ВЫПОЛНЕНО]', '')
        cur = {'cmd': cmd, 'trace': pending}; pending = None
        steps.append(cur)
    elif line.startswith('STATE ') and cur is not None:
        cur['state'] = line[6:].split(' page=')[0].split()
def run(cmds):
    # -w1000: без переноса строк, иначе подпись ссылки может разорваться
    r = subprocess.run(['./instead-cli', '-e', '-w1000', GAME], cwd=CLI, input=('\n'.join(cmds + ['/quit']) + '\n').encode(),
                       capture_output=True, timeout=300)
    return r.stdout.decode('utf-8', 'replace')
icmds = []
for st in steps:
    c, tr = st['cmd'], st['trace']
    if c.startswith('c:'):
        text = c[2:]
        # путь (way): номер из строки «>>», которую instead-cli печатает после пустой команды
        # nlbnoop; ссылка в тексте (xact) — номер из вывода /look
        found = False
        for probe_cmd in ('nlbnoop', '/look'):
            screen = run(icmds + [probe_cmd]).split('\n> ')[-2]
            m = re.findall(re.escape(text) + r'\((\d+)\)', screen.replace('\n', ' '))
            if m:
                icmds += [probe_cmd, m[0]]; found = True; break
        if not found:
            print('не нашёл ссылку:', c, 'шаг', steps.index(st) + 1); break
    elif c == 'l': icmds.append('/look')
    elif tr and tr[0] == 'act': icmds.append('/actid ' + tr[1])
    elif tr and tr[0] == 'inv': icmds.append('/invid ' + tr[1])
    elif tr and tr[0] == 'use': icmds.append('/useid ' + tr[1] + ' ' + tr[2])
    st['icount'] = len(icmds)
    if '--step' in sys.argv:
        out1 = run(icmds + ['/var life energy courage'])
        v = re.findall(r'NLBVARS life=(\S+) energy=(\S+) courage=(\S+)', out1)
        ins = [re.sub(r'\.0$', '', x) for x in v[-1]] if v else ['?'] * 3
        if ins != st.get('state'):
            print('РАСХОЖДЕНИЕ на шаге', steps.index(st) + 1, c, 'INSTEAD', ins, 'плеер', st.get('state'))
            print(out1.split('\n> ')[-3][-1200:])
            break
# один прогон со снимком переменных после каждого шага
cmds = []
k = 0
for st in steps:
    while k < st.get('icount', k): cmds.append(icmds[k]); k += 1
    cmds.append('/var life energy courage')
out = run(cmds)
vals = re.findall(r'NLBVARS life=(\S+) energy=(\S+) courage=(\S+)', out)
norm = lambda v: re.sub(r'\.0$', '', v)
bad = 0
for i, st in enumerate(steps):
    ins = [norm(x) for x in vals[i]] if i < len(vals) else ['?'] * 3
    ours = st.get('state', ['?'] * 3)
    mark = '' if ins == ours else '   <-- РАСХОЖДЕНИЕ'
    if mark and bad < 5:
        print(f'{i+1:>3} {st["cmd"][:45]:45} INSTEAD {" ".join(ins):14} плеер {" ".join(ours)}{mark}')
    bad += bool(mark)
print(f'шагов: {len(steps)}, совпало: {len(steps) - bad}, расхождений: {bad}')
print('КОНЕЦ в INSTEAD' if 'Вы поняли, что делать здесь вам больше нечего' in out else 'в INSTEAD до конца не дошли')
open('instead_walk.txt', 'w', encoding='utf-8').write(out)
