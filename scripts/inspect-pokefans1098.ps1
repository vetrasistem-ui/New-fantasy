param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [switch]$Build
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$AssetRoot = Join-Path $Root 'Game/Assets/Legacy/PokeFans1098'
$ImportRoot = Join-Path $Root 'Game/Imports/Legacy/PokeFans1098'
$Dat = Join-Path $AssetRoot 'Tibia.dat'
$Spr = Join-Path $AssetRoot 'Tibia.spr'
$Otb = Join-Path $AssetRoot 'items.otb'
$Otbm = Join-Path $ImportRoot 'global_dash.otbm'
$Houses = Join-Path $ImportRoot 'map-house.xml'
$Spawns = Join-Path $ImportRoot 'map-spawn.xml'
$BuildDir = Join-Path $Root 'build/studio'
$ReportDir = Join-Path $Root 'build/legacy-pokefans1098'
$Report = Join-Path $ReportDir 'inspect.txt'

$Required = @($Dat, $Spr, $Otb, $Otbm, $Houses, $Spawns)
$Missing = $Required | Where-Object { -not (Test-Path $_) }
if ($Missing) {
    Write-Host 'PokeFans 10.98 local pack is incomplete. Missing:' -ForegroundColor Yellow
    $Missing | ForEach-Object { Write-Host "  $_" }
    Write-Host ''
    Write-Host 'Expected local-only layout:'
    Write-Host '  Game/Assets/Legacy/PokeFans1098/Tibia.dat'
    Write-Host '  Game/Assets/Legacy/PokeFans1098/Tibia.spr'
    Write-Host '  Game/Assets/Legacy/PokeFans1098/items.otb'
    Write-Host '  Game/Imports/Legacy/PokeFans1098/global_dash.otbm'
    Write-Host '  Game/Imports/Legacy/PokeFans1098/map-house.xml'
    Write-Host '  Game/Imports/Legacy/PokeFans1098/map-spawn.xml'
    exit 2
}

$Before = @{}
foreach ($File in $Required) {
    $Before[$File] = (Get-FileHash -Algorithm SHA256 $File).Hash
}

$Candidates = @(
    (Join-Path $BuildDir "$Configuration/fantasy-legacy-inspect.exe"),
    (Join-Path $BuildDir 'fantasy-legacy-inspect.exe')
)
$Inspect = $Candidates | Where-Object { Test-Path $_ } | Select-Object -First 1

if ($Build -or -not $Inspect) {
    Push-Location $Root
    try {
        cmake -S Studio -B build/studio
        if ($LASTEXITCODE -ne 0) { throw "Fantasy Studio configure failed. exit=$LASTEXITCODE" }
        cmake --build build/studio --config $Configuration --target fantasy-legacy-inspect fantasy-otbm-reader-tests fantasy-legacy-binary-readers-tests
        if ($LASTEXITCODE -ne 0) { throw "Legacy compatibility build failed. exit=$LASTEXITCODE" }
        ctest --test-dir build/studio -C $Configuration -R 'fantasy-(legacy-binary-readers|otbm-reader)-tests' --output-on-failure
        if ($LASTEXITCODE -ne 0) { throw "Legacy compatibility tests failed. exit=$LASTEXITCODE" }
    }
    finally {
        Pop-Location
    }
    $Inspect = $Candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
}

if (-not $Inspect) { throw 'fantasy-legacy-inspect.exe was not found after build.' }

New-Item -ItemType Directory -Force -Path $ReportDir | Out-Null
$Output = & $Inspect --dat $Dat --spr $Spr --otb $Otb --otbm $Otbm 2>&1
$Exit = $LASTEXITCODE
$Output | Tee-Object -FilePath $Report
if ($Exit -ne 0) { throw "PokeFans 10.98 inspection failed. exit=$Exit" }

foreach ($File in $Required) {
    $After = (Get-FileHash -Algorithm SHA256 $File).Hash
    if ($After -ne $Before[$File]) {
        throw "Source file changed during read-only inspection: $File"
    }
}

Write-Host ''
Write-Host 'PokeFans 10.98 read-only inspection PASS.' -ForegroundColor Green
Write-Host "Report: $Report"
Write-Host 'Source hashes are unchanged.'
