#!/usr/bin/env python3
"""Страницы книги NLB по каталогу: модули, концовки и возвраты из модулей — без запуска плеера.

    python3 tools/nlb_pages.py <каталог книги>            таблица и итог
    python3 tools/nlb_pages.py <каталог книги> --json     то же в JSON (для скриптов покрытия)

Каждая страница книги и всех её модулей (внутренних — pages/<id>/module/, внешних —
modules/<имя>/) получает вид:

  module   — страница-контейнер модуля (в неё входят через модуль; не концовка);
  return   — возврат из модуля в родительский: страница лежит в модуле и у неё задан rettext,
             autoret или use_mpl (PageImpl::shouldReturn; поля в корне книги не действуют);
  ending   — концовка: нет исходящих ссылок, не контейнер, не возврат, и уйти с неё нечем;
  ending?  — то же, но уйти может быть чем (поле notes): GOTO в модификациях страницы или её
             объектов, объекты на странице, autowired-страницы в книге (autopgs) — такую
             страницу проверяет только прохождение (finished в state-log, docs/state-log.md);
  page     — обычная страница со ссылками.

Правило — как в плеере (PlayerEngine::buildLinks: возврат — отдельный переход LinkKind::Return;
PlayerEngine::isFinished — игра окончена, когда ходов нет). Признак страницы isFinish (nlb_main
info) для этого не годится: он смотрит только rettext, а не autoret/use_mpl, и не знает, лежит ли
страница в модуле. В VN конец наступает, когда нет ни одной ссылки, включая возврат, — объекты там
ход не продолжают, поэтому для VN-книг «ending?» из-за объектов — тоже концовка.
"""
import json
import os
import sys


def read(path):
    try:
        with open(path, encoding='utf-8') as f:
            return f.read().strip()
    except OSError:
        return ''


def has_text(path):
    """Каталог с текстом на каком-нибудь языке (rettext/, caption/…) и непустым содержимым"""
    return os.path.isdir(path) and any(read(os.path.join(path, f)) for f in os.listdir(path))


def caption(page_dir):
    d = os.path.join(page_dir, 'caption')
    if os.path.isdir(d):
        for lang in ['ru', 'en'] + sorted(os.listdir(d)):
            t = read(os.path.join(d, lang))
            if t:
                return t.splitlines()[0]
    return ''


def gotos(item_dir):
    """Число модификаций GOTO у страницы или объекта"""
    d = os.path.join(item_dir, 'modifications')
    if not os.path.isdir(d):
        return 0
    return sum(1 for m in os.listdir(d) if read(os.path.join(d, m, 'type')) == 'GOTO')


class Walker:
    def __init__(self, root):
        self.root = os.path.abspath(root)
        self.rows = []
        self.missing = []

    def objects(self, book_dir):
        """Объекты книги (модуля): id страницы -> [число объектов, GOTO в их модификациях]"""
        result = {}
        d = os.path.join(book_dir, 'objs')
        if os.path.isdir(d):
            for oid in os.listdir(d):
                owner = read(os.path.join(d, oid, 'containerid'))
                if owner:
                    entry = result.setdefault(owner, [0, 0])
                    entry[0] += 1
                    entry[1] += gotos(os.path.join(d, oid))
        return result

    def external(self, name, book_dirs):
        """Внешний модуль ищется в modules/ своей книги, затем объемлющих (findExternalModule)"""
        for b in reversed(book_dirs):
            p = os.path.join(b, 'modules', name)
            if os.path.isdir(os.path.join(p, 'pages')):
                return p
        return None

    def walk(self, book_dir, book_dirs, chain, autowired):
        """book_dirs — каталоги настоящих книг (корень и внешние модули) от корня; chain — путь
        модулей (id страниц-контейнеров); autowired — есть ли autowired-страницы в этой книге
        или выше"""
        pages_dir = os.path.join(book_dir, 'pages')
        if not os.path.isdir(pages_dir):
            return
        autowired = autowired or bool(read(os.path.join(book_dir, 'autopgs')))
        objs = self.objects(book_dir)
        for pid in sorted(os.listdir(pages_dir)):
            p = os.path.join(pages_dir, pid)
            if not os.path.isdir(p):
                continue
            links_dir = os.path.join(p, 'links')
            links = len([x for x in os.listdir(links_dir) if os.path.isdir(os.path.join(links_dir, x))]) if os.path.isdir(links_dir) else 0
            extmod = read(os.path.join(p, 'extmod')) == 'true'
            modname = read(os.path.join(p, 'modname'))
            module_dir = None
            if extmod and modname:
                module_dir = self.external(modname, book_dirs)
                if module_dir is None:
                    self.missing.append(modname)
            elif os.path.isdir(os.path.join(p, 'module', 'pages')) and os.listdir(os.path.join(p, 'module', 'pages')):
                module_dir = os.path.join(p, 'module')
            in_module = len(chain) > 0
            rettext = has_text(os.path.join(p, 'rettext'))
            autoret = read(os.path.join(p, 'autoret')) == 'true'
            use_mpl = read(os.path.join(p, 'use_mpl')) == 'true'
            n_objs, obj_gotos = objs.get(pid, [0, 0])
            page_gotos = gotos(p)
            notes = []
            if page_gotos or obj_gotos:
                notes.append('goto')
            if n_objs:
                notes.append('objects')
            if autowired:
                notes.append('autowired')
            if module_dir or (extmod and modname):
                kind = 'module'
            elif in_module and (rettext or autoret or use_mpl):
                kind = 'return'
            elif links == 0:
                kind = 'ending?' if notes else 'ending'
            else:
                kind = 'page'
            self.rows.append({
                'id': pid,
                'kind': kind,
                'caption': caption(p),
                'depth': len(chain),
                'modules': list(chain),
                'path': os.path.relpath(p, self.root),
                'links': links,
                'autotrav': read(os.path.join(p, 'autotrav')) == 'true',
                'rettext': rettext,
                'autoret': autoret,
                'use_mpl': use_mpl,
                'retpage': read(os.path.join(p, 'retpage')),
                'extmod': modname if extmod else '',
                'notes': notes if kind in ('ending', 'ending?') else [],
            })
            if module_dir:
                real = book_dirs + [module_dir] if extmod else book_dirs
                self.walk(module_dir, real, chain + [pid], autowired)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    if len(args) != 1 or not os.path.isdir(os.path.join(args[0], 'pages')):
        print(__doc__.strip().split('\n\n')[1], file=sys.stderr)
        sys.exit(2)
    w = Walker(args[0])
    w.walk(w.root, [w.root], [], False)
    counts = {}
    for r in w.rows:
        counts[r['kind']] = counts.get(r['kind'], 0) + 1
    if '--json' in sys.argv:
        json.dump({'book': w.root, 'pages': w.rows, 'counts': counts, 'missing_modules': w.missing},
                  sys.stdout, ensure_ascii=False, indent=1)
        print()
        return
    for r in w.rows:
        if r['kind'] == 'page':
            continue
        extra = []
        if r['kind'] == 'return':
            extra = [f for f in ('rettext', 'autoret', 'use_mpl') if r[f]] + (['retpage=' + r['retpage'][:8]] if r['retpage'] else [])
        elif r['kind'] == 'module':
            extra = ['extmod=' + r['extmod']] if r['extmod'] else []
        elif r['notes']:
            extra = r['notes']
        print('%-8s %s %s%s  %s' % (r['kind'], r['id'][:8], '  ' * r['depth'], r['caption'][:50],
                                    ' '.join(extra)))
    print('всего страниц: %d; %s' % (len(w.rows), ', '.join('%s %d' % kv for kv in sorted(counts.items()))))
    if w.missing:
        print('нет внешних модулей (клон без --recurse-submodules?): ' + ', '.join(sorted(set(w.missing))))


if __name__ == '__main__':
    main()
