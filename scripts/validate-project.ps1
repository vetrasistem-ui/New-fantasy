$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$ManifestPath = Join-Path $Root 'fantasy.project.json'
$SchemaPath = Join-Path $Root 'Shared/Formats/Project/schema-v2.json'

function Assert-True {
    param(
        [Parameter(Mandatory = $true)][bool]$Condition,
        [Parameter(Mandatory = $true)][string]$Message
    )
    if (-not $Condition) {
        throw "Project validation failed: $Message"
    }
}

Assert-True (Test-Path $ManifestPath) "manifest not found: $ManifestPath"
Assert-True (Test-Path $SchemaPath) "project schema not found: $SchemaPath"

try {
    $Manifest = Get-Content $ManifestPath -Raw | ConvertFrom-Json -Depth 100
    $Schema = Get-Content $SchemaPath -Raw | ConvertFrom-Json -Depth 100
}
catch {
    throw "Project JSON parse failed: $($_.Exception.Message)"
}

Assert-True ($Schema.title -eq 'Fantasy Project Manifest v2') 'unexpected project schema title'
Assert-True ($Manifest.schemaVersion -eq 2) 'schemaVersion must be 2'
Assert-True (-not [string]::IsNullOrWhiteSpace([string]$Manifest.name)) 'name is required'
Assert-True ($Manifest.protocol -eq 'fantasy-v1') 'protocol must be fantasy-v1'
Assert-True ($Manifest.runtime.server -eq 'FantasyServer') 'runtime.server must be FantasyServer'
Assert-True ($Manifest.runtime.mapFormat -eq 'FMAP-v0') 'runtime.mapFormat must be FMAP-v0'
Assert-True ($Manifest.runtime.protocol -eq 'FantasyProtocol-v1') 'runtime.protocol must be FantasyProtocol-v1'
Assert-True ($Manifest.runtime.authoritativeServer -eq $true) 'runtime.authoritativeServer must be true'
Assert-True ($Manifest.references.role -eq 'reference-only') 'legacy references must be reference-only'

$RequiredPaths = @(
    'studio', 'game', 'mainMap', 'content', 'assets', 'scripts', 'config',
    'server', 'client', 'shared', 'protocolSpec', 'mapSchema',
    'database', 'tools', 'projects'
)

$SeenTargets = @{}
foreach ($Name in $RequiredPaths) {
    $Property = $Manifest.paths.PSObject.Properties[$Name]
    Assert-True ($null -ne $Property) "paths.$Name is required"

    $Relative = [string]$Property.Value
    Assert-True (-not [string]::IsNullOrWhiteSpace($Relative)) "paths.$Name cannot be empty"
    Assert-True (-not [System.IO.Path]::IsPathRooted($Relative)) "paths.$Name must be relative: $Relative"

    $Segments = $Relative -split '[\\/]'
    Assert-True (-not ($Segments -contains '..')) "paths.$Name cannot escape project root: $Relative"

    $Target = [System.IO.Path]::GetFullPath((Join-Path $Root $Relative))
    $RootFull = [System.IO.Path]::GetFullPath($Root)
    Assert-True ($Target.StartsWith($RootFull, [System.StringComparison]::OrdinalIgnoreCase)) "paths.$Name resolves outside project root: $Relative"
    Assert-True (Test-Path $Target) "paths.$Name target does not exist: $Relative"

    if ($Name -in @('mainMap', 'protocolSpec', 'mapSchema')) {
        Assert-True (-not (Test-Path $Target -PathType Container)) "paths.$Name must point to a file: $Relative"
    }

    $Normalized = $Target.TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar)
    if ($Name -notin @('game', 'content', 'assets', 'scripts', 'config')) {
        Assert-True (-not $SeenTargets.ContainsKey($Normalized)) "duplicate project target for paths.$Name and paths.$($SeenTargets[$Normalized]): $Relative"
        $SeenTargets[$Normalized] = $Name
    }
}

Assert-True ([string]$Manifest.paths.mainMap -eq 'Game/Maps/World/world.fmap.json') 'mainMap must use the canonical FMAP path during F01'
Assert-True ([string]$Manifest.paths.protocolSpec -eq 'Shared/Protocol/protocol-v1.yaml') 'protocolSpec must use the canonical path during F01'
Assert-True ([string]$Manifest.paths.mapSchema -eq 'Shared/Formats/FMAP/schema-v0.json') 'mapSchema must use the canonical path during F01'

Write-Host "Fantasy Project v2 validation PASS. name='$($Manifest.name)' root='$Root'"
Write-Host "mainMap=$($Manifest.paths.mainMap)"
Write-Host "protocol=$($Manifest.runtime.protocol) map=$($Manifest.runtime.mapFormat)"
