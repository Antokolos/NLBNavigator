#!/usr/bin/env python3
"""Пошаговая сверка NLBNavigator с настоящим INSTEAD без окна: замена отладочного драйвера drive.

Сценарий по подписям (c:/o:/i:/u:, как в nlb_sdl --script) проигрывается:
  1. в nlb_sdl (SDL_VIDEODRIVER=offscreen, изолированный HOME) с --state-log: после каждой команды —
     страница, переменные и модель экрана (из модели до команды берутся id объектов для o:/i:/u:);
  2. в instead-cli на копии экспорта со служебными командами (make_overlay.sh): c: — номер ссылки
     или пути по её тексту (nlbnoop / /look), o: — /actid <id>, i: — /invid <id>, u: — /useid <id> <id>.
После каждого шага сравниваются страница (/nlbhere и pageId) и переменные (/var и --state-vars).

Использование:
  cmpwalk_sdl.py --player build/nlb_sdl --book <каталог книги> --game <копия экспорта>
                 --cli <каталог release с instead-cli> --script <сценарий> --vars life,energy,courage
                 [--mode standard|vn] [--lang ru] [--out <каталог>]

Ограничения:
  - только экспорты, где текст и ссылки выводятся в консоль: STANDARD (у VN-экспорта текст и выборы
    рисуются спрайтами — см. README);
  - o:/i: — первая подпись, начинающаяся с текста (как в nlb_sdl); одинаковые подписи («кран» и
    «кран») различить нельзя — номер «#2» из log2script.py здесь не поддерживается;
  - объект или место, заданные переменной, и клоны объектов не сопоставляются.
Код выхода: 0 — всё совпало, 1 — есть расхождения, 2 — ошибка запуска.
"""
import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path


def parse_args():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--player', required=True, help='nlb_sdl')
    p.add_argument('--book', required=True, help='каталог книги NLB')
    p.add_argument('--game', required=True, help='копия экспорта INSTEAD со служебными командами')
    p.add_argument('--cli', required=True, help='каталог release с instead-cli')
    p.add_argument('--script', required=True, help='сценарий: c:/o:/i:/u:, по команде в строке')
    p.add_argument('--vars', required=True, help='переменные книги через запятую')
    p.add_argument('--mode', default='standard')
    p.add_argument('--lang', default='ru')
    p.add_argument('--out', default='cmpwalk-out')
    p.add_argument('--timeout', type=int, default=600)
    return p.parse_args()


def read_script(path):
    commands = []
    for line in Path(path).read_text(encoding='utf-8').splitlines():
        line = line.strip()
        if line and not line.startswith('#'):
            if not re.match(r'^[coiu]:', line):
                sys.exit('команда не поддерживается сверкой (нужны c:/o:/i:/u:): ' + line)
            commands.append(line)
    return commands


def run_player(args, commands, out):
    home = out / 'home'
    shutil.rmtree(home, ignore_errors=True)
    home.mkdir(parents=True)
    script = out / 'player.script'
    script.write_text('\n'.join(commands) + '\n', encoding='utf-8')
    state = out / 'state-log.jsonl'
    env = dict(os.environ, HOME=str(home), XDG_CONFIG_HOME=str(home / '.config'),
               XDG_DATA_HOME=str(home / '.local/share'), SDL_VIDEODRIVER='offscreen', SDL_AUDIODRIVER='dummy')
    cmd = [str(Path(args.player).resolve()), str(Path(args.book).resolve()), args.mode, '--no-sound',
           '--lang', args.lang, '--window', '1920x1080', '--script', str(script.resolve()),
           '--state-log', str(state.resolve()), '--state-vars', args.vars]
    r = subprocess.run(cmd, env=env, capture_output=True, text=True, encoding='utf-8', errors='replace',
                       timeout=args.timeout)
    (out / 'player.out').write_text(r.stdout + r.stderr, encoding='utf-8')
    if not state.is_file():
        sys.exit('nlb_sdl не записал state-log (код {}), см. {}'.format(r.returncode, out / 'player.out'))
    records = [json.loads(line) for line in state.read_text(encoding='utf-8').splitlines() if line.strip()]
    if len(records) != len(commands) + 1:
        print('ВНИМАНИЕ: записей state-log {} вместо {}: nlb_sdl остановился на невыполненной команде'
              .format(len(records), len(commands) + 1))
    return records


def find_id(objects, label):
    for o in objects:
        if o.get('label', '').startswith(label):
            return o['id']
    return None


def run_cli(args, cmds):
    r = subprocess.run(['./instead-cli', '-e', '-w1000', str(Path(args.game).resolve())], cwd=args.cli,
                       input=('\n'.join(cmds + ['/quit']) + '\n').encode(), capture_output=True,
                       timeout=args.timeout)
    return r.stdout.decode('utf-8', 'replace')


def link_number(screen, text):
    flat = screen.replace('\n', ' ')
    m = re.findall(re.escape(text) + r'\((\d+)\)', flat)  # точная подпись
    if not m:  # подпись, начинающаяся с текста (как c: в nlb_sdl)
        m = re.findall(re.escape(text) + r'[^()|]*?\((\d+)\)', flat)
    return m[0] if m else None


def norm(v):
    if v is None or v is False or v == 'nil' or v == 'false':
        return 'false'
    if v is True or v == 'true':
        return 'true'
    if isinstance(v, float) and v.is_integer():
        v = int(v)
    return re.sub(r'\.0$', '', str(v))


def page_name(page_id):
    return 'v_' + page_id.replace('-', '_')


def main():
    args = parse_args()
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    names = [v for v in args.vars.split(',') if v]
    commands = read_script(args.script)
    records = run_player(args, commands, out)

    # команды instead-cli для каждого шага; для c: номер ссылки ищется на экране перед шагом
    icmds, marks = [], []
    for i, cmd in enumerate(commands[:len(records) - 1]):
        if not records[i + 1].get('ok', True):
            print('шаг {}: плеер не выполнил «{}» — сценарий не подходит книге, сверка до этого шага'
                  .format(i + 1, cmd))
            break
        before = records[i]['model']
        kind, rest = cmd[0], cmd[2:]
        if kind == 'c':
            number = None
            for probe in ('nlbnoop', '/look'):
                screen = run_cli(args, icmds + [probe]).split('\n> ')[-2]
                number = link_number(screen, rest)
                if number:
                    icmds += [probe, number]
                    break
            if not number:
                print('шаг {}: в INSTEAD нет ссылки «{}»'.format(i + 1, rest))
                break
        elif kind == 'o' or kind == 'i':
            obj = find_id(before['sceneObjects' if kind == 'o' else 'inventory'], rest)
            if not obj:
                print('шаг {}: в модели плеера нет объекта «{}»'.format(i + 1, rest))
                break
            icmds.append(('/actid ' if kind == 'o' else '/invid ') + obj)
        else:
            a, _, b = rest.partition('>')
            ida = find_id(before['inventory'], a)
            idb = find_id(before['sceneObjects'], b) or find_id(before['inventory'], b)
            if not ida or not idb:
                print('шаг {}: в модели плеера нет объектов «{}»'.format(i + 1, rest))
                break
            icmds.append('/useid {} {}'.format(ida, idb))
        marks.append(len(icmds))

    # один прогон INSTEAD: после каждого шага — страница и переменные
    full, k = [], 0
    for m in marks:
        full += icmds[k:m] + ['/nlbhere', '/var ' + ' '.join(names)]
        k = m
    output = run_cli(args, full)
    (out / 'instead.out').write_text(output, encoding='utf-8')
    heres = re.findall(r'NLBHERE (\S+)', output)
    values = re.findall(r'NLBVARS (.*)', output)

    bad = 0
    for i, cmd in enumerate(commands[:len(marks)]):
        rec = records[i + 1]
        diffs = []
        if not rec.get('ok', True):
            diffs.append('плеер не выполнил команду')
        here = heres[i] if i < len(heres) else '?'
        if here != page_name(rec['pageId']):
            diffs.append('страница: INSTEAD {} / плеер {}'.format(here, page_name(rec['pageId'])))
        got = dict(re.findall(r'(\w+)=(.*?)(?= \w+=|$)', values[i])) if i < len(values) else {}
        for n in names:
            a, b = norm(got.get(n, '?')), norm(rec.get('variables', {}).get(n))
            if a != b:
                diffs.append('{}: INSTEAD {} / плеер {}'.format(n, a, b))
        print('{:>3} {:32} {}'.format(i + 1, cmd[:32], 'совпадает' if not diffs else 'РАСХОЖДЕНИЕ: ' + '; '.join(diffs)))
        bad += bool(diffs)
    total = len(commands)
    print('шагов: {}, сверено: {}, с расхождениями: {}'.format(total, len(marks), bad))
    sys.exit(0 if bad == 0 and len(marks) == total else 1)


if __name__ == '__main__':
    main()
