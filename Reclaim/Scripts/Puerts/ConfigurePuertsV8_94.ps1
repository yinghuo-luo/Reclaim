param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$UProject = Join-Path $ProjectRoot "Reclaim.uproject"
if (-not (Test-Path $UProject)) {
    throw "Reclaim.uproject not found at '$ProjectRoot'. Run this script from the baseline project tree or pass -ProjectRoot."
}

$PuertsRoot = Join-Path $ProjectRoot "Plugins\Puerts"
$JsEnvBuild = Join-Path $PuertsRoot "Source\JsEnv\JsEnv.Build.cs"
$V8Folder = Join-Path $PuertsRoot "ThirdParty\v8_9.4.146.24"

if (-not (Test-Path $JsEnvBuild)) {
    throw "Missing Puerts plugin: $JsEnvBuild`nCopy your already-downloaded Puerts plugin to Reclaim\Plugins\Puerts first."
}
if (-not (Test-Path $V8Folder)) {
    throw "Missing V8 9.4.146.24 backend: $V8Folder"
}

$text = Get-Content $JsEnvBuild -Raw
if ($text -notmatch "V9_4_146_24") {
    throw "This Puerts source does not expose SupportedV8Versions.V9_4_146_24."
}

$backup = "$JsEnvBuild.baseline-backup"
if (-not (Test-Path $backup)) {
    Copy-Item $JsEnvBuild $backup
}

$pattern = '(?ms)(#if\s+UE_4_25_OR_LATER\s*\r?\n\s*)SupportedV8Versions\.[A-Za-z0-9_]+;'
if ($text -notmatch $pattern) {
    throw "Could not locate the UE_4_25_OR_LATER V8 selector in JsEnv.Build.cs; refusing to guess-edit it."
}

$text = [regex]::Replace($text, $pattern, '${1}SupportedV8Versions.V9_4_146_24;', 1)
$text = [regex]::Replace($text, 'private\s+bool\s+UseNodejs\s*=\s*true\s*;', 'private bool UseNodejs = false;')
$text = [regex]::Replace($text, 'private\s+bool\s+UseQuickjs\s*=\s*true\s*;', 'private bool UseQuickjs = false;')
Set-Content -Path $JsEnvBuild -Value $text -Encoding UTF8

Write-Host "Puerts backend configured for V8 9.4.146.24." -ForegroundColor Green
Write-Host "Verified: $V8Folder"
