#!/usr/bin/env python3
# Создаёт Private/Generated/*.cpp — по обёртке на каждый .cpp плеера, чтобы UBT собрал их в модуле
# NLBPlayer. Каждая обёртка — отдельная единица трансляции (в Build.cs отключена unity-сборка).
# Запускать после добавления или удаления файлов в Source/NLBCore, NLBReader, NLBPresentation.
import os, shutil
here = os.path.dirname(os.path.abspath(__file__))
src = os.path.abspath(os.path.join(here, '..'))
gen = os.path.join(here, 'Private', 'Generated')
shutil.rmtree(gen, ignore_errors=True)
os.makedirs(gen)
# из cparse — только сама библиотека (без тестов catch.cpp, test-shunting-yard.cpp)
CPARSE = ('shunting-yard.cpp', 'packToken.cpp', 'functions.cpp', 'containers.cpp')
count = 0
for sub in ('NLBCore/Private', 'NLBReader/Private', 'NLBPresentation/Private', 'thirdparty/cparse'):
    for d, _, files in sorted(os.walk(os.path.join(src, sub))):
        for f in sorted(files):
            if not f.endswith('.cpp'):
                continue
            if sub == 'thirdparty/cparse' and f not in CPARSE:
                continue
            rel = os.path.relpath(os.path.join(d, f), src).replace(os.sep, '/')
            with open(os.path.join(gen, 'NLB_' + rel.replace('/', '_')), 'w', encoding='utf-8') as out:
                out.write('// Сгенерировано update_sources.py — не редактировать\n#include "%s"\n' % rel)
            count += 1
print(gen, count, 'файлов')
