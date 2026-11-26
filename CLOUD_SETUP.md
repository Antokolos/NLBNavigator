# Локальные настройки облачной среды NLB-Developer

Команды используют новый уникальный каталог, оставляя JDK, Maven и
пользовательский профиль вне области записи.

```bash
source /workspace/nlb-developer-env-v1/activate.sh
work=$(mktemp -d /workspace/nlb-local.XXXXXXXX)
mkdir "$work/java-home" "$work/window-home"
cp "$JAVA_HOME/lib/security/cacerts" "$work/cacerts"
"$JAVA_HOME/bin/keytool" -importcert -noprompt -alias cloud-proxy \
  -file "$CODEX_PROXY_CERT" -keystore "$work/cacerts" -storepass changeit
python3 - "$work/settings.xml" <<'PY'
import os
import sys
from urllib.parse import urlsplit
from xml.etree.ElementTree import Element, ElementTree, SubElement

proxy = urlsplit(os.getenv("https_proxy") or os.environ["HTTPS_PROXY"])
if proxy.scheme != "http" or not proxy.hostname or proxy.username is not None:
    raise SystemExit("Ожидался HTTP-прокси среды без встроенных учётных данных")
root = Element("settings")
entry = SubElement(SubElement(root, "proxies"), "proxy")
for key, value in {
    "id": "cloud", "active": "true", "protocol": "http",
    "host": proxy.hostname, "port": str(proxy.port or 80),
}.items():
    SubElement(entry, key).text = value
ElementTree(root).write(sys.argv[1], encoding="utf-8")
PY
proxy_opts=$(python3 -c 'import os; from urllib.parse import urlsplit; p=urlsplit(os.getenv("https_proxy") or os.environ["HTTPS_PROXY"]); print(f"-Dhttps.proxyHost={p.hostname} -Dhttps.proxyPort={p.port or 80}")')
export JAVA_TOOL_OPTIONS="-Duser.home=$work/java-home -Djavax.net.ssl.trustStore=$work/cacerts $proxy_opts"
export XDG_CONFIG_HOME="$work/java-home/.config"
mvn -s "$work/settings.xml" -B -ntp -C -U \
  -Dmaven.resolver.transport=wagon -Dmaven.repo.local="$work/m2" package
```

Последнюю команду запускайте из отдельной распакованной копии проверяемого
NLB HEAD. Для тестов укажите `NLB_TEST_BOOKS` с закреплёнными книгами.
Используйте CA, предоставленный облачной средой в `CODEX_PROXY_CERT`.

## Графические проверки

В отдельную копию `$NLB_ENV/debian/etc/xdg/openbox/rc.xml` добавьте перед
`</applications>` правило `<application class="*"><decor>no</decor><maximized>yes</maximized></application>`.
Для NLBNavigator запустите этот экземпляр Openbox в том же дисплее, что и
раннер. Для INSTEAD штатный раннер запускает и останавливает свой Openbox.

```bash
export MAGICK_THREAD_LIMIT=2 OMP_NUM_THREADS=2 SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=x11
mkdir -p "$work/window-home"/{.config,.cache,tmp}
env HOME="$work/window-home" XDG_CONFIG_HOME="$work/window-home/.config" \
  XDG_CACHE_HOME="$work/window-home/.cache" TMPDIR="$work/window-home/tmp" \
  xvfb-run -a -s '-screen 0 1920x1080x24 -noreset' bash -c '
    cfg=$1
    shift
    openbox --sm-disable --config-file "$cfg" &
    wm=$!
    trap '\''kill -TERM "$wm" 2>/dev/null; wait "$wm"'\'' EXIT
    xdpyinfo | grep -E "dimensions:|depth of root window:"
    "$@"
' _ "$work/openbox.xml" python3 tests/differential/run_route.py ...
```

На каждый вызов раннера передавайте новый `--output`. Штатный раннер создаёт
отдельные HOME, XDG, INSTEAD appdata и overlay. Проверяйте размеры кадров
и фактические коды завершения. При нехватке CPU для штатного пятисекундного
таймаута ImageMagick можно использовать `import -define png:compression-level=1
-define png:compression-filter=0`. PNG по-прежнему сохраняется без потерь.
