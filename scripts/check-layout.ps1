$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$Required = @(
    'Studio',
    'Game',
    'Game/Maps',
    'Game/Maps/World',
    'Game/Maps/World/world.fmap.json',
    'Game/Content',
    'Game/Assets',
    'Game/Scripts',
    'Game/Config',
    'Server',
    'Server/CMakeLists.txt',
    'Server/Core/main.cpp',
    'Client',
    'Shared',
    'Shared/Protocol/protocol-v1.yaml',
    'Shared/Formats/FMAP/schema-v0.json',
    'Database',
    'Tools',
    'Projects',
    'docs',
    'fantasy.project.json'
)

$Failed = $false
foreach ($Relative in $Required) {
    $Path = Join-Path $Root $Relative
    if (-not (Test-Path $Path)) {
        Write-Host "[missing] $Relative"
        $Failed = $true
    }
    else {
        Write-Host "[ok] $Relative"
    }
}

$ManifestPath = Join-Path $Root 'fantasy.project.json'
if (Test-Path $ManifestPath) {
    $Manifest = Get-Content $ManifestPath -Raw | ConvertFrom-Json

    foreach ($Property in $Manifest.paths.PSObject.Properties) {
        $Value = [string]$Property.Value
        if ([System.IO.Path]::IsPathRooted($Value)) {
            Write-Host "[absolute-path] $($Property.Name) = $Value"
            $Failed = $true
        }
    }

    if ($Manifest.runtime.mapFormat -notlike 'FMAP*') {
        Write-Host "[legacy-map-format] $($Manifest.runtime.mapFormat)"
        $Failed = $true
    }

    if ($Manifest.runtime.protocol -notlike 'FantasyProtocol*') {
        Write-Host "[legacy-protocol] $($Manifest.runtime.protocol)"
        $Failed = $true
    }

    $MainMap = Join-Path $Root ([string]$Manifest.paths.mainMap)
    if (-not (Test-Path $MainMap)) {
        Write-Host "[missing-main-map] $($Manifest.paths.mainMap)"
        $Failed = $true
    }
}

$FmapPath = Join-Path $Root 'Game/Maps/World/world.fmap.json'
if (Test-Path $FmapPath) {
    try {
        $Fmap = Get-Content $FmapPath -Raw | ConvertFrom-Json
        if ($Fmap.format -ne 'FMAP' -or $Fmap.version -ne 0) {
            Write-Host '[invalid-fmap-header] expected format=FMAP version=0'
            $Failed = $true
        }
    }
    catch {
        Write-Host "[invalid-fmap-json] $($_.Exception.Message)"
        $Failed = $true
    }
}

if ($Failed) {
    throw 'Project layout/contract validation failed.'
}

Write-Host 'Layout and contract validation PASS.'
