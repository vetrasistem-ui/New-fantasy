param(
    [Parameter(Mandatory = $true)]
    [string]$PackRoot,

    [string]$Dat,
    [string]$Spr,
    [string]$Otb,
    [string]$Otbm,
    [string]$OutputDir,
    [string]$Profile = "pokefans1098"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Require-File([string]$Path, [string]$Label) {
    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Label file not found: $Path"
    }
    return (Get-Item -LiteralPath $Path).FullName
}

function Pick-File([string]$Explicit, [string]$Extension, [string]$PreferredName, [string]$Label) {
    if (-not [string]::IsNullOrWhiteSpace($Explicit)) {
        return Require-File $Explicit $Label
    }

    $candidates = @(Get-ChildItem -LiteralPath $PackRoot -Recurse -File -ErrorAction Stop |
        Where-Object { $_.Extension -ieq $Extension })
    if ($candidates.Count -eq 0) {
        throw "No $Label file (*$Extension) was found below $PackRoot"
    }

    if (-not [string]::IsNullOrWhiteSpace($PreferredName)) {
        $preferred = @($candidates | Where-Object { $_.Name -ieq $PreferredName } | Sort-Object Length -Descending)
        if ($preferred.Count -gt 0) {
            Write-Host "AUTO $Label -> $($preferred[0].FullName)"
            return $preferred[0].FullName
        }
    }

    $selected = $candidates | Sort-Object Length -Descending | Select-Object -First 1
    Write-Host "AUTO $Label -> $($selected.FullName)"
    return $selected.FullName
}

$PackRoot = (Resolve-Path -LiteralPath $PackRoot).Path
if ([string]::IsNullOrWhiteSpace($OutputDir)) {
    $OutputDir = Join-Path $PackRoot "FantasyRealAssetPoc"
}
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$OutputDir = (Resolve-Path -LiteralPath $OutputDir).Path

$Scanner = Join-Path $PSScriptRoot "fantasy-asset-candidates.exe"
$Generator = Join-Path $PSScriptRoot "fantasy-map-generate.exe"
Require-File $Scanner "fantasy-asset-candidates.exe" | Out-Null
Require-File $Generator "fantasy-map-generate.exe" | Out-Null

$Dat = Pick-File $Dat ".dat" "Tibia.dat" "DAT"
$Spr = Pick-File $Spr ".spr" "Tibia.spr" "SPR"
$Otb = Pick-File $Otb ".otb" "items.otb" "OTB"
$Otbm = Pick-File $Otbm ".otbm" "global_dash.otbm" "OTBM"

$candidatesPath = Join-Path $OutputDir "asset-candidates.json"
$previewDir = Join-Path $OutputDir "asset-previews"
$bindingsPath = Join-Path $OutputDir "real-asset-bindings.first-map.json"
$scriptPath = Join-Path $OutputDir "fantasy-first-region.fmapcmd"
$catalogPath = Join-Path $OutputDir "asset-catalog.first-map.json"
$summaryPath = Join-Path $OutputDir "selected-assets.first-map.json"
$mapPath = Join-Path $OutputDir "fantasy-first-region.otbm"

Write-Host ""
Write-Host "[1/3] Scanning real DAT/SPR/OTB/OTBM assets..."
$scannerArgs = @(
    "--dat", $Dat,
    "--spr", $Spr,
    "--otb", $Otb,
    "--otbm", $Otbm,
    "--output", $candidatesPath,
    "--preview-dir", $previewDir,
    "--profile", $Profile,
    "--top-grounds", "200",
    "--top-objects", "500"
)
& $Scanner @scannerArgs
if ($LASTEXITCODE -ne 0) {
    throw "Real asset scanner failed with exit code $LASTEXITCODE"
}

$candidates = Get-Content -LiteralPath $candidatesPath -Raw | ConvertFrom-Json
$grounds = @($candidates.grounds | Where-Object { $_.spritesValid -eq $true -and $_.kind -eq "ground" })
$objects = @($candidates.objects | Where-Object {
    $_.spritesValid -eq $true -and ($_.kind -eq "object" -or $_.kind -eq "border")
})

if ($grounds.Count -lt 2) {
    throw "The real pack did not expose at least two valid ground candidates. Inspect $candidatesPath"
}
if ($objects.Count -lt 3) {
    throw "The real pack did not expose at least three valid object/border candidates. Inspect $candidatesPath"
}

$selected = [ordered]@{
    schemaVersion = 1
    profileId = $Profile
    note = "Automatic technical selection from the most-used real assets. These aliases generate the first real Fantasy map, but their visual meaning must be confirmed from BMP previews before renaming them grass/water/tree/wall/door."
    assets = @(
        [ordered]@{ id = "terrain.primary"; serverId = [uint32]$grounds[0].serverId; clientId = [uint32]$grounds[0].clientId; uses = [uint64]$grounds[0].uses; preview = $grounds[0].preview },
        [ordered]@{ id = "terrain.secondary"; serverId = [uint32]$grounds[1].serverId; clientId = [uint32]$grounds[1].clientId; uses = [uint64]$grounds[1].uses; preview = $grounds[1].preview },
        [ordered]@{ id = "object.primary"; serverId = [uint32]$objects[0].serverId; clientId = [uint32]$objects[0].clientId; uses = [uint64]$objects[0].uses; preview = $objects[0].preview },
        [ordered]@{ id = "object.secondary"; serverId = [uint32]$objects[1].serverId; clientId = [uint32]$objects[1].clientId; uses = [uint64]$objects[1].uses; preview = $objects[1].preview },
        [ordered]@{ id = "object.tertiary"; serverId = [uint32]$objects[2].serverId; clientId = [uint32]$objects[2].clientId; uses = [uint64]$objects[2].uses; preview = $objects[2].preview }
    )
}
$selected | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $summaryPath -Encoding UTF8

$bindings = [ordered]@{
    schemaVersion = 1
    profileId = $Profile
    bindings = @(
        [ordered]@{ id = "terrain.primary"; serverId = [uint32]$grounds[0].serverId },
        [ordered]@{ id = "terrain.secondary"; serverId = [uint32]$grounds[1].serverId },
        [ordered]@{ id = "object.primary"; serverId = [uint32]$objects[0].serverId },
        [ordered]@{ id = "object.secondary"; serverId = [uint32]$objects[1].serverId },
        [ordered]@{ id = "object.tertiary"; serverId = [uint32]$objects[2].serverId }
    )
}
$bindings | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $bindingsPath -Encoding UTF8

$mapCommands = @"
# Fantasy Studio - First Generated Map
# 160x160 floor 7 with a starter settlement, vegetation, south terrain band and roads.
# Generic aliases are bound only to VALIDATED real assets from this exact 10.98 pack.
NEW_REGION fantasy_first_region 160 160 7 terrain.primary
PLACE_WATER 0 136 160 24 7 terrain.secondary
PLACE_FOREST 5 8 46 52 7 object.primary 24 1098
PLACE_FOREST 110 8 44 52 7 object.primary 24 2098
PLACE_FOREST 8 92 34 34 7 object.primary 18 3098
PLACE_FOREST 118 92 34 34 7 object.primary 18 4098
PLACE_BUILDING 68 48 14 10 7 terrain.primary object.secondary object.tertiary
PLACE_BUILDING 44 68 12 10 7 terrain.primary object.secondary object.tertiary
PLACE_BUILDING 72 72 16 12 7 terrain.primary object.secondary object.tertiary
PLACE_BUILDING 104 68 12 10 7 terrain.primary object.secondary object.tertiary
PLACE_BUILDING 72 100 16 12 7 terrain.primary object.secondary object.tertiary
CONNECT 79 24 79 135 7 5 terrain.secondary
CONNECT 36 78 124 78 7 5 terrain.secondary
CONNECT 75 57 79 78 7 3 terrain.secondary
CONNECT 50 77 50 78 7 3 terrain.secondary
CONNECT 80 83 80 78 7 3 terrain.secondary
CONNECT 110 77 110 78 7 3 terrain.secondary
CONNECT 80 111 80 78 7 3 terrain.secondary
VALIDATE
SAVE fantasy-first-region.otbm
"@
Set-Content -LiteralPath $scriptPath -Value $mapCommands -Encoding UTF8

Write-Host ""
Write-Host "[2/3] Binding first-map aliases to real serverId/clientId/sprites..."
Write-Host "  terrain.primary   serverId=$($grounds[0].serverId) clientId=$($grounds[0].clientId)"
Write-Host "  terrain.secondary serverId=$($grounds[1].serverId) clientId=$($grounds[1].clientId)"
Write-Host "  object.primary    serverId=$($objects[0].serverId) clientId=$($objects[0].clientId)"
Write-Host "  object.secondary  serverId=$($objects[1].serverId) clientId=$($objects[1].clientId)"
Write-Host "  object.tertiary   serverId=$($objects[2].serverId) clientId=$($objects[2].clientId)"

Write-Host ""
Write-Host "[3/3] Generating the first Fantasy OTBM using only validated real assets..."
$generatorArgs = @(
    "--dat", $Dat,
    "--spr", $Spr,
    "--otb", $Otb,
    "--bindings", $bindingsPath,
    "--script", $scriptPath,
    "--output-root", $OutputDir,
    "--profile", $Profile,
    "--catalog-out", $catalogPath
)
& $Generator @generatorArgs
if ($LASTEXITCODE -ne 0) {
    throw "Real map generator failed with exit code $LASTEXITCODE"
}

if (-not (Test-Path -LiteralPath $mapPath -PathType Leaf)) {
    throw "Generator reported success but the expected OTBM was not created: $mapPath"
}

Write-Host ""
Write-Host "FIRST FANTASY MAP PASS"
Write-Host "Map:        $mapPath"
Write-Host "Script:     $scriptPath"
Write-Host "Candidates: $candidatesPath"
Write-Host "Previews:   $previewDir"
Write-Host "Bindings:   $bindingsPath"
Write-Host "Catalog:    $catalogPath"
Write-Host "Selection:  $summaryPath"
Write-Host ""
Write-Host "The OTBM is real and uses assets validated against this exact DAT/SPR/OTB pack."
Write-Host "The automatic aliases are still technical: inspect BMP previews before assigning semantic names such as grass, water, tree, wall and door."
