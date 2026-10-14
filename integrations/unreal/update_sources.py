#!/usr/bin/env python3
# Генерирует Private/Generated/*.cpp — по одной обёртке на каждый .cpp плеера (ядро, движок,
# NLBPresentation, cparse), чтобы UBT собрал их в модуле NLBNavigatorRuntime. Каждая обёртка —
# отдельная единица трансляции: у файлов плеера есть одноимённые функции в безымянных
# пространствах имён, объединять их в один файл нельзя (и в Build.cs отключена unity-сборка).
# Запускать после добавления или удаления файлов в Source/.
import os, shutil
here = os.path.dirname(os.path.abspath(__file__))
root = os.path.abspath(os.path.join(here, '..', '..'))
src = os.path.join(root, 'Source')
gen = os.path.join(here, 'NLBNavigator', 'Source', 'NLBNavigatorRuntime', 'Private', 'Generated')
shutil.rmtree(gen, ignore_errors=True)
os.makedirs(gen)
count = 0
for sub in ('NLBCore/Private', 'NLBReader/Private', 'NLBPresentation/Private', 'thirdparty/cparse'):
    for d, _, files in sorted(os.walk(os.path.join(src, sub))):
        for f in sorted(files):
            if not f.endswith('.cpp') or 'test' in f.lower():
                continue
            rel = os.path.relpath(os.path.join(d, f), src).replace(os.sep, '/')
            name = rel.replace('/', '_')
            with open(os.path.join(gen, name), 'w', encoding='utf-8') as out:
                out.write('// Сгенерировано update_sources.py — не редактировать\n')
                out.write('#include "%s"\n' % rel)
            count += 1
print(gen, count, 'файлов')
