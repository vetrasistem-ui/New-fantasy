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
$bindingsPath = Join-Path $OutputDir "real-asset-bindings.smoke.json"
$scriptPath = Join-Path $OutputDir "real-assets-smoke.fmapcmd"
$catalogPath = Join-Path $OutputDir "asset-catalog.smoke.json"
$summaryPath = Join-Path $OutputDir "selected-assets.smoke.json"
$mapPath = Join-Path $OutputDir "real-assets-smoke.otbm"

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
    note = "Automatic smoke selection from the most-used real assets. These aliases prove the real pipeline; classify the previews later before assigning names such as grass/water/tree/wall/door."
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
# Automatic real-asset smoke map. OTBM stays the output/source compatibility format.
# Aliases are intentionally generic until the generated BMP previews are classified.
NEW_REGION real_asset_smoke 64 64 7 terrain.primary
SET_TERRAIN 8 8 18 12 7 terrain.secondary
PLACE_FOREST 2 2 60 60 7 object.primary 8 1098
PLACE_BUILDING 24 24 16 12 7 terrain.secondary object.secondary object.tertiary
CONNECT 4 48 58 48 7 3 terrain.secondary
VALIDATE
SAVE real-assets-smoke.otbm
"@
Set-Content -LiteralPath $scriptPath -Value $mapCommands -Encoding UTF8

Write-Host ""
Write-Host "[2/3] Binding selected aliases to real serverId/clientId/sprites..."
Write-Host "  terrain.primary   serverId=$($grounds[0].serverId) clientId=$($grounds[0].clientId)"
Write-Host "  terrain.secondary serverId=$($grounds[1].serverId) clientId=$($grounds[1].clientId)"
Write-Host "  object.primary    serverId=$($objects[0].serverId) clientId=$($objects[0].clientId)"
Write-Host "  object.secondary  serverId=$($objects[1].serverId) clientId=$($objects[1].clientId)"
Write-Host "  object.tertiary   serverId=$($objects[2].serverId) clientId=$($objects[2].clientId)"

Write-Host ""
Write-Host "[3/3] Generating an OTBM using only validated real assets..."
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
Write-Host "REAL ASSET POC PASS"
Write-Host "Map:        $mapPath"
Write-Host "Candidates: $candidatesPath"
Write-Host "Previews:   $previewDir"
Write-Host "Bindings:   $bindingsPath"
Write-Host "Catalog:    $catalogPath"
Write-Host "Selection:  $summaryPath"
Write-Host ""
Write-Host "This smoke map proves the real DAT/SPR/OTB -> semantic binding -> Fantasy commands -> OTBM path."
Write-Host "Next, inspect the BMP previews and replace generic aliases with curated names such as terrain.grass, terrain.water and nature.tree."
