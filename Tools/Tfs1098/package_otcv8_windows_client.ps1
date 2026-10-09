param(
    [Parameter(Mandatory = $true)]
    [string]$SourceRoot,

    [Parameter(Mandatory = $true)]
    [string]$OutputRoot,

    [string]$AssetsRoot = ""
)

$ErrorActionPreference = 'Stop'

function Require-Path {
    param([string]$Path, [string]$Description)
    if (-not (Test-Path -LiteralPath $Path)) {
        throw "Missing ${Description}: $Path"
    }
}

$source = (Resolve-Path -LiteralPath $SourceRoot).Path
$output = [System.IO.Path]::GetFullPath($OutputRoot)

Require-Path (Join-Path $source 'otclient_dx.exe') 'OTCv8 DirectX executable'
Require-Path (Join-Path $source 'otclient_gl.exe') 'OTCv8 OpenGL executable'
Require-Path (Join-Path $source 'init.lua') 'OTCv8 init.lua'
Require-Path (Join-Path $source 'modules') 'OTCv8 modules directory'
Require-Path (Join-Path $source 'data') 'OTCv8 data directory'

if (Test-Path -LiteralPath $output) {
    Remove-Item -LiteralPath $output -Recurse -Force
}
New-Item -ItemType Directory -Path $output -Force | Out-Null

foreach ($file in @(
    'otclient_dx.exe',
    'otclient_gl.exe',
    'd3dcompiler_47.dll',
    'libEGL.dll',
    'libGLESv2.dll',
    'init.lua'
)) {
    $candidate = Join-Path $source $file
    if (Test-Path -LiteralPath $candidate) {
        Copy-Item -LiteralPath $candidate -Destination (Join-Path $output $file) -Force
    }
}

foreach ($directory in @('modules', 'mods', 'data', 'layouts')) {
    $candidate = Join-Path $source $directory
    if (Test-Path -LiteralPath $candidate) {
        Copy-Item -LiteralPath $candidate -Destination (Join-Path $output $directory) -Recurse -Force
    }
}

# Never inherit arbitrary Tibia assets from the public/source package. Assets must
# come from the user-owned 10.98 pack supplied explicitly to this tool.
$thingsDirectory = Join-Path $output 'data\things'
if (Test-Path -LiteralPath $thingsDirectory) {
    Remove-Item -LiteralPath $thingsDirectory -Recurse -Force
}
New-Item -ItemType Directory -Path (Join-Path $thingsDirectory '1098') -Force | Out-Null

$assetsIncluded = $false
if (-not [string]::IsNullOrWhiteSpace($AssetsRoot)) {
    $assets = (Resolve-Path -LiteralPath $AssetsRoot).Path
    $dat = Join-Path $assets 'Tibia.dat'
    $spr = Join-Path $assets 'Tibia.spr'
    Require-Path $dat 'Tibia.dat'
    Require-Path $spr 'Tibia.spr'

    Copy-Item -LiteralPath $dat -Destination (Join-Path $thingsDirectory '1098\Tibia.dat') -Force
    Copy-Item -LiteralPath $spr -Destination (Join-Path $thingsDirectory '1098\Tibia.spr') -Force

    foreach ($optional in @('Tibia.otfi', 'Tibia.otml', 'things.otml')) {
        $candidate = Join-Path $assets $optional
        if (Test-Path -LiteralPath $candidate) {
            Copy-Item -LiteralPath $candidate -Destination (Join-Path $thingsDirectory "1098\$optional") -Force
        }
    }
    $assetsIncluded = $true
}

$sourceNote = @"
source=https://github.com/OTCv8/otclientv8
platform=windows
clientVersion=1098
assetsIncluded=$($assetsIncluded.ToString().ToLowerInvariant())
"@
Set-Content -LiteralPath (Join-Path $output 'HOMOLOGATION-SOURCE.txt') -Value $sourceNote -Encoding ascii

if (-not $assetsIncluded) {
    $assetNote = @"
Fantasy 10.98 assets were intentionally NOT packaged.

To complete a local visual acceptance, rerun this tool with -AssetsRoot pointing to a user-owned matching 10.98 asset directory containing:
  Tibia.dat
  Tibia.spr

Optional profile files such as Tibia.otfi/Tibia.otml are copied when present.
"@
    Set-Content -LiteralPath (Join-Path $output 'ASSETS-REQUIRED.txt') -Value $assetNote -Encoding ascii
}

$dx = Get-Item -LiteralPath (Join-Path $output 'otclient_dx.exe')
$gl = Get-Item -LiteralPath (Join-Path $output 'otclient_gl.exe')
if ($dx.Length -le 0 -or $gl.Length -le 0) {
    throw 'Packaged OTCv8 executable is empty.'
}

$manifest = [ordered]@{
    schemaVersion = 1
    client = 'otcv8'
    protocol = 1098
    platform = 'windows'
    preferredExecutable = 'otclient_dx.exe'
    alternateExecutable = 'otclient_gl.exe'
    assetsIncluded = $assetsIncluded
    generatedAtUtc = [DateTime]::UtcNow.ToString('o')
}
$manifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'fantasy-client-manifest.json') -Encoding utf8

Write-Host "FANTASY_OTCV8_WINDOWS_PACKAGE PASS"
Write-Host "output=$output"
Write-Host "assetsIncluded=$assetsIncluded"
Write-Host "dxBytes=$($dx.Length)"
Write-Host "glBytes=$($gl.Length)"
