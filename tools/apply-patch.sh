#!/usr/bin/env bash
# Применить патчи Разработчика UE (git format-patch) по правилам веток NLBNavigator.
#
#   tools/apply-patch.sh main <тема> файлы.patch…   патч для main: ветка ue-dev/<тема> от
#                                                  origin/main, git am, push и PR в main
#                                                  (main защищён — только через PR); если ветка
#                                                  уже есть (исправление по ревью) — патчи
#                                                  ложатся поверх неё, PR тот же
#   tools/apply-patch.sh UE файлы.patch…           патч только для игры: git am прямо в UE и push
#   tools/apply-patch.sh merge                     после слияния PR в main: git merge origin/main
#                                                  в UE и push (все коммиты main — и в UE, всегда)
#
# PR открывается через gh (GitHub CLI), если он есть и вошёл в GitHub; иначе скрипт печатает
# адрес, по которому PR создаётся одной кнопкой. Рабочее дерево должно быть чистым; при ошибке
# git am применение отменяется, и скрипт возвращает на ветку, с которой начинали.
# Удалённый репозиторий — origin (переменная REMOTE меняет его).
set -euo pipefail

REMOTE=${REMOTE:-origin}

die() { echo "apply-patch: $*" >&2; exit 1; }

usage() {
    sed -n '2,15p' "$0" | sed 's/^# \{0,1\}//'
    exit 2
}

[ $# -ge 1 ] || usage
target=$1; shift

git rev-parse --git-dir >/dev/null 2>&1 || die "запускать внутри репозитория NLBNavigator"
cd "$(git rev-parse --show-toplevel)"
[ -z "$(git status --porcelain --untracked-files=no)" ] || die "в рабочем дереве есть изменения — сначала закоммитьте или уберите их (git stash)"
[ ! -d "$(git rev-parse --git-path rebase-apply)" ] || die "не закончено прежнее git am (git am --continue или git am --abort)"

start=$(git symbolic-ref --quiet --short HEAD || git rev-parse HEAD)
back() { git switch --quiet "$start" 2>/dev/null || git checkout --quiet "$start" 2>/dev/null || true; }

check_patches() {
    [ $# -ge 1 ] || die "не указаны файлы патчей"
    local p
    for p in "$@"; do
        [ -f "$p" ] || die "нет файла: $p"
    done
}

# Пути патчей — абсолютные: после git switch текущий каталог тот же, но так надёжнее
abs_patches=()
abs() {
    local p
    for p in "$@"; do
        abs_patches+=("$(cd "$(dirname "$p")" && pwd)/$(basename "$p")")
    done
}

# ветка, созданная этим запуском: при ошибке git am — удаляется
new_branch=""

am() {
    if ! git am --3way "${abs_patches[@]}"; then
        git am --abort || true
        back
        [ -z "$new_branch" ] || git branch --quiet -D "$new_branch" || true
        die "патч не применился (см. выше) — ничего не изменено; возможно, он для другой ветки (main/UE) или сделан не от свежей"
    fi
}

# owner/repo из адреса origin (https://github.com/o/r.git, git@github.com:o/r.git)
github_repo() {
    local url
    url=$(git remote get-url "$REMOTE")
    case "$url" in
        *github.com[:/]*) url=${url#*github.com}; url=${url#[:/]}; echo "${url%.git}" ;;
        *) echo "" ;;
    esac
}

git fetch --quiet "$REMOTE"

case "$target" in
    main)
        [ $# -ge 2 ] || die "нужны тема и файлы: tools/apply-patch.sh main <тема> файлы.patch…"
        topic=$1; shift
        case "$topic" in
            *[!A-Za-z0-9._-]*|"") die "тема — латиница, цифры, «.», «_», «-» (станет веткой ue-dev/<тема>): $topic" ;;
        esac
        check_patches "$@"; abs "$@"
        branch="ue-dev/$topic"
        if git ls-remote --exit-code --heads "$REMOTE" "$branch" >/dev/null 2>&1; then
            # исправление по ревью: поверх уже открытой ветки
            echo "Ветка $branch уже есть на $REMOTE — патчи ложатся поверх неё"
            git switch --quiet -C "$branch" "$REMOTE/$branch"
            existing=1
        else
            git switch --quiet -C "$branch" "$REMOTE/main"
            existing=0
            new_branch=$branch
        fi
        am
        git push --quiet -u "$REMOTE" "$branch"
        repo=$(github_repo)
        if [ "$existing" = 1 ]; then
            echo "Добавлено в ветку $branch — открытый PR обновится сам."
        elif [ -n "$repo" ] && command -v gh >/dev/null 2>&1 && gh auth status >/dev/null 2>&1; then
            # заголовок и описание — из сообщения первого коммита
            first=$(git rev-list --reverse "$REMOTE/main..HEAD" | head -n 1)
            gh pr create --repo "$repo" --base main --head "$branch" \
                --title "$(git log -1 --format=%s "$first")" \
                --body "$(git log --reverse --format='### %s%n%n%b' "$REMOTE/main..HEAD")"
        elif [ -n "$repo" ]; then
            echo "Открыть PR: https://github.com/$repo/compare/main...$branch?expand=1"
        else
            echo "Ветка $branch отправлена в $REMOTE; откройте PR в main."
        fi
        back
        echo "Когда «Differential gate» зелёный и PR слит: tools/apply-patch.sh merge"
        ;;
    UE)
        check_patches "$@"; abs "$@"
        git switch --quiet UE 2>/dev/null || git switch --quiet -c UE --track "$REMOTE/UE"
        git merge --quiet --ff-only "$REMOTE/UE" || { back; die "локальная UE разошлась с $REMOTE/UE — разберитесь вручную"; }
        am
        git push --quiet "$REMOTE" UE
        back
        echo "Патч применён в UE и отправлен."
        ;;
    merge)
        [ $# -eq 0 ] || die "merge без аргументов"
        git switch --quiet UE 2>/dev/null || git switch --quiet -c UE --track "$REMOTE/UE"
        git merge --quiet --ff-only "$REMOTE/UE" || { back; die "локальная UE разошлась с $REMOTE/UE — разберитесь вручную"; }
        if git merge-base --is-ancestor "$REMOTE/main" HEAD; then
            back
            echo "UE уже содержит весь main — сливать нечего."
            exit 0
        fi
        if ! git merge --no-edit "$REMOTE/main"; then
            echo "apply-patch: конфликт при слиянии main в UE. Разрешите его в UE (git status), затем" >&2
            echo "git commit и git push $REMOTE UE; или отмените: git merge --abort" >&2
            exit 1
        fi
        git push --quiet "$REMOTE" UE
        back
        echo "main влит в UE и отправлен."
        ;;
    -h|--help) usage ;;
    *) die "первый аргумент — main, UE или merge (tools/apply-patch.sh --help)" ;;
esac
