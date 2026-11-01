<#
.SYNOPSIS
  Расшифровать падение NLBNavigator на Android: журнал -> функции, файлы и строки.

.DESCRIPTION
  Принимает журнал (adb logcat, в любой кодировке — UTF-16 из PowerShell тоже) или файл
  журнала игры NLBNavigator.log. Сам находит в нём текст ошибки и стек (строки «[Callstack]»
  движка или «#NN pc … libUnreal.so» системы), находит в каталоге out\ библиотеку с символами
  от той же сборки (по Build ID, если он есть в журнале; иначе — самую свежую) и расшифровывает
  адреса через llvm-addr2line из NDK.

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File tools\ue\symbolize_android_crash.ps1 phone.txt
  powershell -ExecutionPolicy Bypass -File tools\ue\symbolize_android_crash.ps1 -Capture
    (-Capture: сам снимает журнал с подключённого телефона: adb logcat -d)
#>
param(
    [string]$Log = "",
    [switch]$Capture,
    [string]$Ndk = $(if ($env:ANDROID_NDK_ROOT) { $env:ANDROID_NDK_ROOT } else { "" }),
    [string]$Symbols = ""
)
$ErrorActionPreference = "Stop"
$ProjectDir = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path

# --- журнал
if ($Capture) {
    $Log = Join-Path $ProjectDir "out\crash_logcat.txt"
    & adb logcat -d | Set-Content -Encoding UTF8 $Log
    Write-Host "Журнал снят: $Log"
}
if (-not $Log -or -not (Test-Path $Log)) { throw "Укажите файл журнала или -Capture" }
$text = [IO.File]::ReadAllText((Resolve-Path $Log))   # .NET сам узнаёт UTF-16/UTF-8 по BOM

# --- NDK: llvm-addr2line
if (-not $Ndk) {
    $sdk = if ($env:ANDROID_HOME) { $env:ANDROID_HOME } else { Join-Path $env:LOCALAPPDATA "Android\Sdk" }
    $Ndk = Get-ChildItem (Join-Path $sdk "ndk") -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending | Select-Object -First 1 -ExpandProperty FullName
}
$bin = Join-Path $Ndk "toolchains\llvm\prebuilt\windows-x86_64\bin"
$addr2line = Join-Path $bin "llvm-addr2line.exe"
$readelf = Join-Path $bin "llvm-readelf.exe"
if (-not (Test-Path $addr2line)) { throw "Нет llvm-addr2line в $bin — укажите -Ndk <каталог NDK>" }

# --- ошибка и адреса
Write-Host "`n=== Ошибка ==="
$text -split "`n" | Where-Object { $_ -match "Fatal error|Assertion failed|Ensure condition|unrecognized pointer|Fatal signal|=== Critical error" } |
    Select-Object -First 8 | ForEach-Object { Write-Host ($_.Trim()) }
$addrs = @([regex]::Matches($text, 'libUnreal\.so\(0x([0-9A-Fa-f]+)\)') | ForEach-Object { "0x" + $_.Groups[1].Value.TrimStart('0') })
if ($addrs.Count -eq 0) {
    $addrs = @([regex]::Matches($text, '#\d+\s+pc\s+([0-9a-fA-F]+)\s+\S*libUnreal\.so') | ForEach-Object { "0x" + $_.Groups[1].Value.TrimStart('0') })
}
$addrs = $addrs | Select-Object -Unique | Select-Object -First 40
if ($addrs.Count -eq 0) { throw "В журнале нет стека libUnreal.so (падение не в игре или журнал неполный)" }
$buildId = ([regex]::Match($text, 'libUnreal\.so \(BuildId: ([0-9a-f]+)\)')).Groups[1].Value

# --- библиотека с символами той же сборки
if (-not $Symbols) {
    $candidates = Get-ChildItem (Join-Path $ProjectDir "out"), (Join-Path $ProjectDir "Binaries\Android") -Recurse -Include "libUnreal.so", "NLBNavigator-arm64*.so" -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending
    foreach ($c in $candidates) {
        $id = (& $readelf -n $c.FullName | Select-String "Build ID: ([0-9a-f]+)").Matches.Groups[1].Value
        if (-not $buildId -or $id -eq $buildId) { $Symbols = $c.FullName; break }
    }
    if (-not $Symbols) { throw "Нет библиотеки с символами для Build ID $buildId — эта сборка уже перезаписана; пересоберите, повторите падение и запустите снова" }
}
Write-Host "`n=== Символы: $Symbols $(if ($buildId) { "(Build ID $buildId — совпадает)" } else { '(Build ID в журнале нет — взята самая свежая сборка)' })"

# --- расшифровка
Write-Host "`n=== Стек (сверху — место падения) ===`n"
& $addr2line -C -f -i -p -e $Symbols @addrs | ForEach-Object { Write-Host $_ }
