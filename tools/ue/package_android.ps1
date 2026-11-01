<#
.SYNOPSIS
  Упаковать NLBNavigator (Unreal) для Android: шлем Meta Quest (VR) или обычный телефон.

.DESCRIPTION
  Один проект — две сборки:
    quest — как настроено в проекте: OpenXR, пакет для Meta Quest, только VR;
    phone — без OpenXR и без пометок Quest: обычный режим, экранные джойстики и кнопки.
  Для phone скрипт временно меняет Config/DefaultEngine.ini (bPackageForMetaQuest,
  ExtraApplicationSettings) и NLBNavigator.uproject (плагин OpenXR выключен), упаковывает и
  возвращает файлы как были — даже если упаковка упала.

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File tools\ue\package_android.ps1 -Target phone
  powershell -ExecutionPolicy Bypass -File tools\ue\package_android.ps1 -Target quest -Config Shipping
#>
param(
    [ValidateSet("phone", "quest")] [string]$Target = "phone",
    [ValidateSet("Development", "Shipping")] [string]$Config = "Development",
    # установленная сборка движка (каталог с Engine\Build\BatchFiles\RunUAT.bat)
    [string]$Engine = $(if ($env:UE_ROOT) { $env:UE_ROOT } else { "C:\work\UE\UnrealEngine\LocalBuilds\Engine\Windows" }),
    [string]$Out = ""
)
$ErrorActionPreference = "Stop"
$Project = (Resolve-Path (Join-Path $PSScriptRoot "..\..\NLBNavigator.uproject")).Path
$ProjectDir = Split-Path $Project
if (-not $Out) { $Out = Join-Path $ProjectDir "out\$Target" }
$RunUAT = Join-Path $Engine "Engine\Build\BatchFiles\RunUAT.bat"
$EditorCmd = Join-Path $Engine "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
if (-not (Test-Path $RunUAT)) { throw "Нет $RunUAT — укажите -Engine или переменную UE_ROOT" }

$EngineIni = Join-Path $ProjectDir "Config\DefaultEngine.ini"

# Книга попадает в APK только из Content\NLBFiles (отдельными файлами, DefaultGame.ini). Если она
# лежит в NLBFiles в корне проекта (так её находит игра на ПК) — скопировать в Content\NLBFiles
$BookInContent = Join-Path $ProjectDir "Content\NLBFiles"
$BookInRoot = Join-Path $ProjectDir "NLBFiles"
if (-not (Get-ChildItem $BookInContent -ErrorAction SilentlyContinue)) {
    if (Get-ChildItem $BookInRoot -ErrorAction SilentlyContinue) {
        Write-Host "Книга: копирую NLBFiles -> Content\NLBFiles (в APK попадает только оттуда)"
        Copy-Item $BookInRoot $BookInContent -Recurse -Force
    } else {
        throw "Нет книги: положите её в Content\NLBFiles (например, Content\NLBFiles\book.nlbpack)"
    }
}
$Backups = @()
function Backup($path) {
    $copy = "$path.nlbbak"
    # Не затираем резервную копию, оставшуюся после прерванного предыдущего запуска.
    if (Test-Path $copy) {
        throw "Уже существует резервная копия $copy. Проверьте её перед повторным запуском."
    }
    Copy-Item $path $copy
    $script:Backups += ,@($path, $copy)
}

try {
    if ($Target -eq "phone") {
        Backup $EngineIni
        Backup $Project
        # пакет без пометок Quest (иначе телефон не установит или магазин сочтёт приложением для VR)
        $ini = Get-Content $EngineIni -Raw -Encoding UTF8
        $ini = $ini -replace '(?m)^bPackageForMetaQuest=.*$', 'bPackageForMetaQuest=False'
        $ini = $ini -replace '(?m)^ExtraApplicationSettings=.*$', 'ExtraApplicationSettings='
        # не у всех телефонов Vulkan, подходящий движку: запасной рендер — OpenGL ES 3.2
        $ini = $ini -replace '(?m)^bBuildForES31=.*$', 'bBuildForES31=True'
        # Отрисовка для шлема (две картинки за проход) телефону не нужна, а шейдеры под неё у
        # части телефонов дают чёрную сцену при обычном (одном) виде — выключить
        $ini = $ini -replace '(?m)^vr\.MobileMultiView=.*$', 'vr.MobileMultiView=False'
        $ini = $ini -replace '(?m)^vr\.InstancedStereo=.*$', 'vr.InstancedStereo=False'
        [IO.File]::WriteAllText($EngineIni, $ini, (New-Object Text.UTF8Encoding $false))
        # без OpenXR: в игре нет системы XR — она сама включает обычный режим и экранное управление
        $json = Get-Content $Project -Raw -Encoding UTF8 | ConvertFrom-Json
        foreach ($p in $json.Plugins) { if ($p.Name -like "OpenXR*") { $p.Enabled = $false } }
        [IO.File]::WriteAllText($Project, ($json | ConvertTo-Json -Depth 20), (New-Object Text.UTF8Encoding $false))
        Write-Host "Сборка для телефона: Meta Quest и OpenXR выключены на время упаковки"
    } else {
        Write-Host "Сборка для Meta Quest: настройки проекта как есть"
    }

    & $RunUAT BuildCookRun -utf8output -nop4 -nocompileeditor -skipbuildeditor -installed `
        -project="$Project" -target=NLBNavigator -unrealexe="$EditorCmd" `
        -platform=Android -cookflavor=ASTC "-clientconfig=$Config" `
        -build -cook -stage -pak -iostore -compressed -package -archive -archivedirectory="$Out" -prereqs
    if ($LASTEXITCODE -ne 0) { throw "Упаковка не удалась (код $LASTEXITCODE)" }
    Write-Host "Готово: $Out"
}
finally {
    foreach ($b in $Backups) {
        Copy-Item $b[1] $b[0] -Force
        Remove-Item $b[1] -Force
    }
    if ($Backups.Count -gt 0) { Write-Host "Настройки проекта возвращены" }
}
