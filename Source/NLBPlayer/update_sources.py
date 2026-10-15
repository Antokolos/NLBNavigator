#!/usr/bin/env python3
# Создаёт Private/Generated/*.cpp — по обёртке на каждый .cpp плеера, чтобы UBT собрал их в модуле
# NLBPlayer. Список файлов берётся из CMakeLists.txt (NLB_ALL_SOURCES): в Source/NLBCore лежат и
# устаревшие файлы, которые CMake не собирает (LinkLw.cpp, UndoManager.cpp, …), — их брать нельзя.
# Каждая обёртка — отдельная единица трансляции (в Build.cs отключена unity-сборка).
# Запускать после изменения списка исходников в CMakeLists.txt.
import os, re, shutil
here = os.path.dirname(os.path.abspath(__file__))
root = os.path.abspath(os.path.join(here, '..', '..'))
cmake = open(os.path.join(root, 'CMakeLists.txt'), encoding='utf-8').read()
dirs = {
    'NLB_SOURCE_DIR': 'NLBCore/Private',
    'NLB_READER_SOURCE_DIR': 'NLBReader/Private',
}
sources = []
for var, sub in dirs.items():
    sources += [sub + '/' + f for f in re.findall(r'\$\{' + var + r'\}/(\S+?\.cpp)', cmake)]
sources += ['NLBPresentation/Private/' + f for f in re.findall(r'Source/NLBPresentation/Private/(\S+?\.cpp)', cmake)]
# из cparse — только сама библиотека (без тестов catch.cpp, test-shunting-yard.cpp)
sources += ['thirdparty/cparse/' + f for f in ('shunting-yard.cpp', 'packToken.cpp', 'functions.cpp', 'containers.cpp')]
seen = set()
gen = os.path.join(here, 'Private', 'Generated')
shutil.rmtree(gen, ignore_errors=True)
os.makedirs(gen)
for rel in sources:
    if rel in seen:
        continue
    seen.add(rel)
    if not os.path.exists(os.path.join(root, 'Source', rel)):
        raise SystemExit('нет файла ' + rel)
    with open(os.path.join(gen, 'NLB_' + rel.replace('/', '_')), 'w', encoding='utf-8') as out:
        out.write('// Сгенерировано update_sources.py — не редактировать\n#include "%s"\n' % rel)
print(gen, len(seen), 'файлов')
