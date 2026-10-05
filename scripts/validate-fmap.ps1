$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$SchemaPath = Join-Path $Root 'Shared/Formats/FMAP/schema-v0.json'
$MapPath = Join-Path $Root 'Game/Maps/World/world.fmap.json'

function Assert-True {
    param(
        [Parameter(Mandatory = $true)][bool]$Condition,
        [Parameter(Mandatory = $true)][string]$Message
    )

    if (-not $Condition) {
        throw "FMAP validation failed: $Message"
    }
}

function Is-IntegerValue {
    param($Value)
    return ($Value -is [byte] -or $Value -is [sbyte] -or $Value -is [int16] -or $Value -is [uint16] -or $Value -is [int32] -or $Value -is [uint32] -or $Value -is [int64] -or $Value -is [uint64])
}

Assert-True (Test-Path $SchemaPath) "schema not found: $SchemaPath"
Assert-True (Test-Path $MapPath) "map not found: $MapPath"

try {
    $Schema = Get-Content $SchemaPath -Raw | ConvertFrom-Json -Depth 100
    $Map = Get-Content $MapPath -Raw | ConvertFrom-Json -Depth 100
}
catch {
    throw "FMAP JSON parse failed: $($_.Exception.Message)"
}

Assert-True ($Schema.title -eq 'Fantasy Map Format v0') 'unexpected schema title'
Assert-True ($Map.format -eq 'FMAP') 'format must be FMAP'
Assert-True ($Map.version -eq 0) 'version must be 0'
Assert-True ($null -ne $Map.world) 'world is required'
Assert-True (-not [string]::IsNullOrWhiteSpace([string]$Map.world.id)) 'world.id is required'
Assert-True (-not [string]::IsNullOrWhiteSpace([string]$Map.world.name)) 'world.name is required'
Assert-True ((Is-IntegerValue $Map.world.tileSize) -and [int64]$Map.world.tileSize -gt 0) 'world.tileSize must be a positive integer'

foreach ($Axis in @('x', 'y', 'z')) {
    Assert-True ((Is-IntegerValue $Map.developmentSpawn.$Axis)) "developmentSpawn.$Axis must be an integer"
}

Assert-True ($null -ne $Map.regions) 'regions is required'
Assert-True ($Map.regions.Count -gt 0) 'at least one region is required'

$RegionIds = @{}
$TotalChunks = 0
$TotalTiles = 0
$SpawnTagMatches = 0
$SemanticIdPattern = '^[a-z0-9]+(?:[._-][a-z0-9]+)*$'

foreach ($Region in $Map.regions) {
    $RegionId = [string]$Region.id
    Assert-True (-not [string]::IsNullOrWhiteSpace($RegionId)) 'region.id is required'
    Assert-True (-not $RegionIds.ContainsKey($RegionId)) "duplicate region id: $RegionId"
    $RegionIds[$RegionId] = $true

    foreach ($Axis in @('x', 'y', 'z')) {
        Assert-True ((Is-IntegerValue $Region.origin.$Axis)) "region '$RegionId' origin.$Axis must be an integer"
    }

    Assert-True ((Is-IntegerValue $Region.size.width) -and [int64]$Region.size.width -gt 0) "region '$RegionId' width must be positive"
    Assert-True ((Is-IntegerValue $Region.size.height) -and [int64]$Region.size.height -gt 0) "region '$RegionId' height must be positive"
    Assert-True ($null -ne $Region.chunks) "region '$RegionId' chunks is required"

    $ChunkKeys = @{}
    foreach ($Chunk in $Region.chunks) {
        $TotalChunks++
        foreach ($Field in @('x', 'y', 'floor')) {
            Assert-True ((Is-IntegerValue $Chunk.$Field)) "region '$RegionId' chunk.$Field must be an integer"
        }

        $ChunkKey = "$($Chunk.x):$($Chunk.y):$($Chunk.floor)"
        Assert-True (-not $ChunkKeys.ContainsKey($ChunkKey)) "duplicate chunk '$ChunkKey' in region '$RegionId'"
        $ChunkKeys[$ChunkKey] = $true
        Assert-True ($null -ne $Chunk.tiles) "chunk '$ChunkKey' tiles is required"

        $TileKeys = @{}
        foreach ($Tile in $Chunk.tiles) {
            $TotalTiles++
            Assert-True ((Is-IntegerValue $Tile.x)) "tile.x in '$RegionId/$ChunkKey' must be an integer"
            Assert-True ((Is-IntegerValue $Tile.y)) "tile.y in '$RegionId/$ChunkKey' must be an integer"
            Assert-True ([int64]$Tile.x -ge 0 -and [int64]$Tile.y -ge 0) "tile coordinates in '$RegionId/$ChunkKey' must be non-negative"
            Assert-True (-not [string]::IsNullOrWhiteSpace([string]$Tile.ground)) "tile ground in '$RegionId/$ChunkKey' is required"
            Assert-True ([string]$Tile.ground -match $SemanticIdPattern) "invalid ground semantic id '$($Tile.ground)'"

            $TileKey = "$($Tile.x):$($Tile.y)"
            Assert-True (-not $TileKeys.ContainsKey($TileKey)) "duplicate tile '$TileKey' in '$RegionId/$ChunkKey'"
            $TileKeys[$TileKey] = $true

            foreach ($ObjectId in @($Tile.objects)) {
                Assert-True (-not [string]::IsNullOrWhiteSpace([string]$ObjectId)) "empty object semantic id in '$RegionId/$ChunkKey/$TileKey'"
                Assert-True ([string]$ObjectId -match $SemanticIdPattern) "invalid object semantic id '$ObjectId'"
            }

            foreach ($Tag in @($Tile.tags)) {
                Assert-True (-not [string]::IsNullOrWhiteSpace([string]$Tag)) "empty tag in '$RegionId/$ChunkKey/$TileKey'"
            }

            if (@($Tile.tags) -contains 'development-spawn') {
                $GlobalX = [int64]$Region.origin.x + [int64]$Chunk.x + [int64]$Tile.x
                $GlobalY = [int64]$Region.origin.y + [int64]$Chunk.y + [int64]$Tile.y
                $GlobalZ = [int64]$Chunk.floor
                if ($GlobalX -eq [int64]$Map.developmentSpawn.x -and $GlobalY -eq [int64]$Map.developmentSpawn.y -and $GlobalZ -eq [int64]$Map.developmentSpawn.z) {
                    $SpawnTagMatches++
                }
            }
        }
    }
}

Assert-True ($TotalChunks -gt 0) 'at least one chunk is required'
Assert-True ($TotalTiles -gt 0) 'at least one tile is required'
Assert-True ($SpawnTagMatches -eq 1) "developmentSpawn must resolve to exactly one tile tagged development-spawn; found $SpawnTagMatches"

Write-Host "FMAP v0 validation PASS. regions=$($Map.regions.Count) chunks=$TotalChunks tiles=$TotalTiles"
