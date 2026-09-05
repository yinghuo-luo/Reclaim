param(
    [Parameter(Mandatory = $false)]
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

$ErrorActionPreference = "Stop"
$TsConfig = Join-Path $ProjectRoot "TypeScript\tsconfig.json"
if (-not (Test-Path $TsConfig)) {
    throw "TypeScript config not found: $TsConfig"
}

$Tsc = Get-Command tsc -ErrorAction SilentlyContinue
if ($Tsc) {
    & $Tsc.Source -p $TsConfig
    exit $LASTEXITCODE
}

$Npx = Get-Command npx -ErrorAction SilentlyContinue
if ($Npx) {
    & $Npx.Source tsc -p $TsConfig
    exit $LASTEXITCODE
}

throw "Neither 'tsc' nor 'npx' is available. Compiled JS is committed under Content/JavaScript, so Unreal can still run; install TypeScript tooling before editing .ts files."
