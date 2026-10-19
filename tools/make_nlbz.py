#!/usr/bin/env python3
"""Собрать игру .nlbz для проигрывателя NLBNavigator (Android и др.) и строку каталога к ней.

.nlbz — обычный ZIP (сжатие deflate) из двух файлов:
  book.nlbpack — пакет книги (nlb_main <книга> pack … --with-media --theme … --base-theme …)
  meta.json    — описание игры (те же поля, что у игры в каталоге, кроме ссылок)

Пример:
  python tools/make_nlbz.py book.nlbpack servant-of-ra.nlbz --id servant-of-ra --version 1.0 \\
      --title ru="Слуга Ра" --title en="Servant of Ra" --author ru="…" --mode standard \\
      --url https://example.org/games/servant-of-ra.nlbz

Печатает запись для каталога (catalog.json) с размером архива; ссылки на миниатюру и скриншот
добавьте в неё сами (--thumbnail, --screenshot).
"""
import argparse
import json
import os
import sys
import zipfile


def lang_map(values):
    """['ru=Слуга Ра', 'en=Servant of Ra'] -> {'ru': 'Слуга Ра', 'en': 'Servant of Ra'}"""
    out = {}
    for v in values or []:
        if '=' not in v:
            sys.exit(f'ожидается язык=текст: {v}')
        lang, text = v.split('=', 1)
        out[lang.strip()] = text
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('pack', help='пакет книги .nlbpack')
    ap.add_argument('out', help='архив игры .nlbz')
    ap.add_argument('--id', required=True, help='идентификатор игры (латиница, без пробелов)')
    ap.add_argument('--version', default='1.0')
    ap.add_argument('--title', action='append', help='название: язык=текст (можно несколько)')
    ap.add_argument('--author', action='append', help='автор: язык=текст')
    ap.add_argument('--summary', action='append', help='краткое описание: язык=текст')
    ap.add_argument('--description', action='append', help='полное описание: язык=текст')
    ap.add_argument('--mode', default='vn', choices=['vn', 'standard'], help='режим книги')
    ap.add_argument('--args', default='', help='дополнительные ключи плеера (например, --lang ru)')
    ap.add_argument('--url', default='', help='ссылка на скачивание .nlbz (для записи каталога)')
    ap.add_argument('--thumbnail', default='', help='ссылка на миниатюру (для каталога)')
    ap.add_argument('--screenshot', default='', help='ссылка на скриншот (для каталога)')
    a = ap.parse_args()

    if not os.path.isfile(a.pack):
        sys.exit(f'нет файла {a.pack}')
    meta = {
        'id': a.id,
        'version': a.version,
        'title': lang_map(a.title),
        'author': lang_map(a.author),
        'summary': lang_map(a.summary),
        'description': lang_map(a.description),
        'mode': a.mode,
        'args': a.args,
    }
    if not meta['title']:
        sys.exit('нужно хотя бы одно название: --title ru=…')
    with zipfile.ZipFile(a.out, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        z.writestr('meta.json', json.dumps(meta, ensure_ascii=False, indent=2))
        z.write(a.pack, 'book.nlbpack')
    entry = dict(meta)
    entry.update({'size': os.path.getsize(a.out), 'url': a.url, 'thumbnail': a.thumbnail, 'screenshot': a.screenshot})
    print(json.dumps(entry, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
