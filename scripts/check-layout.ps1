$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$Required = @(
    'Studio',
    'Game',
    'Game/Maps',
    'Game/Content',
    'Game/Assets',
    'Game/Scripts',
    'Game/Config',
    'Server',
    'Client',
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
}

if ($Failed) {
    throw 'Project layout validation failed.'
}

Write-Host 'Layout validation PASS.'
