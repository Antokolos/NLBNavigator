#!/usr/bin/env python3
# log2script.py <лог> <выход> — числовые команды консольного лога -> сценарий по подписям
import re, sys
t = open(sys.argv[1], encoding='utf-8', errors='replace').read().replace('\r', '')
blocks = t.split('Ваш выбор: ')
script = []
for i in range(1, len(blocks)):
    prev = blocks[i-1]; cmd = blocks[i].split('\n', 1)[0].strip()
    choices, objs, inv = {}, {}, {}
    for line in prev.split('\n'):
        m = re.match(r'^(\d+)\. (.*)$', line)
        if m: choices[m.group(1)] = m.group(2)
    # компактный формат (одна строка) или дерево (по строке на объект)
    for line in prev.split('\n'):
        mm = re.match(r'^\s+o(\d+)\. (.*)$', line)
        if mm: objs[mm.group(1)] = re.sub(r' \[[^\]]*\]$', '', mm.group(2).replace(' (можно взять)', '').replace(' (свёрнут)', '')).strip()
        mi = re.match(r'^\s+i(\d+)\. (.*)$', line)
        if mi: inv[mi.group(1)] = mi.group(2).strip()
    m = re.search(r'Здесь:(.*)', prev)
    if m:
        for n, lab in re.findall(r'o(\d+)\. (.*?)(?=\s{2}o\d+\. |$)', m.group(1)): objs[n] = lab.replace(' (можно взять)', '').strip()
    m = re.search(r'Инвентарь:(.*?)(?:\n\n|\n\d+\. |\nl\. )', prev, re.S)
    if m:
        for n, lab in re.findall(r'i(\d+)\. (.*?)(?=\s{2}i\d+\. |$)', m.group(1), re.S): inv[n] = lab.strip().split('\n')[0]
    after = blocks[i].split('\n', 1)[1] if '\n' in blocks[i] else ''
    if after.startswith('Неверный выбор') or after.startswith('Нет варианта'):
        script.append('# неверный ввод: ' + cmd); continue
    def lab(ref):
        d = objs if ref[0] == 'o' else inv
        name = d.get(ref[1:], '?')
        # одинаковые подписи (два «крана») — номер вхождения: «кран#2»
        same = [k for k in sorted(d, key=int) if d[k] == name]
        return name if len(same) < 2 else name + '#' + str(same.index(ref[1:]) + 1)
    if cmd in ('0', 'm'): script.append('# ' + cmd); continue
    if cmd in ('l', 'w'): script.append(cmd); continue
    if re.fullmatch(r'\d+', cmd): script.append('c:' + choices.get(cmd, '?'))
    elif re.fullmatch(r'[oi]\d+', cmd): script.append(('o:' if cmd[0] == 'o' else 'i:') + lab(cmd))
    elif cmd.startswith('u'):
        a, b = cmd[1:].split(); script.append('u:' + inv.get(a, '?') + '>' + lab(b))
    else: script.append('# ?' + cmd)
open(sys.argv[2], 'w').write('\n'.join(script) + '\n')
print(len(script), 'команд;', sum(1 for s in script if '?' in s), 'с неизвестной подписью')
