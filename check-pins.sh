set -euo pipefail
root=/workspace/nlb-acceptance.01Jw9LE0/nav-data/test-data
for entry in wolfman:888b4b7a7a564693274f53341728ee9f3b4c2879 instead-am_i_a_monster_vn:0db8d2c611faa596987baaf041763ef6c9282484 instead:1a07e0ffc0a1e8402b9d47a104d88d3f33204a7e; do
    name=${entry%:*}
    expected=${entry#*:}
    path="$root/$name"
    test "$(realpath -e "$path")" = "$path"
    sha=$(git -C "$path" rev-parse HEAD)
    git -C "$path" status --porcelain=v1 --untracked-files=all --ignore-submodules=none > "$1/$name.status"
    test ! -s "$1/$name.status"
    printf '%s %s\n' "$name" "$sha"
    test "$sha" = "$expected"
done
