set -u
source /workspace/nlb-developer-env-v1/activate.sh
d=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
cd /workspace/NLBNavigator || exit
data=/workspace/nlb-acceptance.01Jw9LE0/nav-data/test-data
export SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=x11 MAGICK_THREAD_LIMIT=2 OMP_NUM_THREADS=2 LP_NUM_THREADS=2
export PATH="$d/capture-tools:$PATH"
if [[ $1 == x11 ]]; then
    out=$2
    shift 2
    xdpyinfo > "$out/nlb.xdpyinfo"
    openbox --sm-disable --config-file "$d/openbox.xml" > "$out/nlb.openbox.log" 2>&1 &
    wm=$!
    trap 'kill -TERM "$wm" 2>/dev/null; wait "$wm"' EXIT
    for ((attempt=0; attempt<30; attempt++)); do
        id=$(xprop -root -notype _NET_SUPPORTING_WM_CHECK | awk '/window id/ {print $NF}')
        [[ -n $id ]] && break
        sleep 0.1
    done
    xprop -id "$id" _NET_WM_NAME > "$out/nlb.wm.log" || exit
    "$@"
    exit $?
fi
kind=$1
label=$2
out="${NLB_RESULT_ROOT:-$d/results}/$label"
mkdir "$out" || exit
export HOME="$out/home" XDG_CONFIG_HOME="$out/home/.config" XDG_CACHE_HOME="$out/home/.cache" XDG_DATA_HOME="$out/home/.local/share" TMPDIR="$out/tmp"
mkdir -p "$XDG_CONFIG_HOME" "$XDG_CACHE_HOME" "$XDG_DATA_HOME" "$TMPDIR"
run() {
    local label=$1 status
    shift
    printf '%q ' "$@" > "$out/$label.command"
    printf '\n' >> "$out/$label.command"
    "$@" > "$out/$label.log" 2>&1
    status=$?
    printf '%s\n' "$status" > "$out/$label.exit"
    printf '%s %s %s\n' "${out##*/}" "$label" "$status"
}
xvfb=(xvfb-run -a -n "$3" -s '-screen 0 1920x1080x24 -noreset')
game="$data/instead-am_i_a_monster_vn"
base="$data/instead/themes/default"
player=(--player "$d/build/nlb_sdl" --book "$data/wolfman" --theme "$game/themes/default" --base-theme "$base")
runner=(python3 tests/differential/run_route.py)
if [[ $kind == pair || $kind == retry-instead ]]; then
    route=$4
    if [[ $kind == retry-instead ]]; then
        original="$d/results/$label"
        test "$(cat "$original/nlb.exit")" = 0 || exit 1
        mkdir "$out/nlbnavigator"
        cp -- "$original/nlbnavigator/"*.png "$original/nlbnavigator/route-report.json" "$original/nlbnavigator/state-log.jsonl" "$out/nlbnavigator/" || exit
        cp -- "$original"/nlb.{exit,log,command} "$out/" || exit
    else
    run nlb "${xvfb[@]}" -e "$out/nlb.xvfb.log" bash "$d/run-local.sh" x11 "$out" "${runner[@]}" --runtime nlbnavigator --timeout 450 --route "$route" "${player[@]}" --output "$out/nlbnavigator"
    fi
    run instead env PATH="$d/slow-input:$PATH" SDL_RENDER_DRIVER=opengl "${xvfb[@]}" -e "$out/instead.xvfb.log" "${runner[@]}" --runtime instead --route "$route" --instead-binary "$(command -v sdl-instead)" --base-theme "$base" --game "$game" --output "$out/instead"
    run compare nice -n 6 "${runner[@]}" --runtime compare --route "$route" --output "$out"
else
    run nlb "${xvfb[@]}" -e "$out/nlb.xvfb.log" bash "$d/run-local.sh" x11 "$out" python3 "tests/differential/check_wolfman_$label.py" "${player[@]}" --output "$out/scenarios" "${@:4}"
fi
