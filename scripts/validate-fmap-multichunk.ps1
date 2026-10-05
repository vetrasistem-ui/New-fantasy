$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$MapPath = Join-Path $Root 'Game/Maps/World/multichunk-fixture.fmap.json'

function Assert-True {
    param(
        [Parameter(Mandatory = $true)][bool]$Condition,
        [Parameter(Mandatory = $true)][string]$Message
    )

    if (-not $Condition) {
        throw "FMAP multi-chunk validation failed: $Message"
    }
}

Assert-True (Test-Path $MapPath) "fixture not found: $MapPath"
$Map = Get-Content $MapPath -Raw | ConvertFrom-Json -Depth 100
Assert-True ($Map.format -eq 'FMAP') 'format must be FMAP'
Assert-True ($Map.version -eq 0) 'version must be 0'

$TotalChunks = 0
$TotalTiles = 0
$CrossChunkPairs = 0
$SpawnMatches = 0

foreach ($Region in $Map.regions) {
    $RegionMinX = [int64]$Region.origin.x
    $RegionMinY = [int64]$Region.origin.y
    $RegionMaxX = $RegionMinX + [int64]$Region.size.width
    $RegionMaxY = $RegionMinY + [int64]$Region.size.height
    $Resolved = @{}

    foreach ($Chunk in $Region.chunks) {
        $TotalChunks++
        Assert-True ([int64]$Chunk.x -ge 0) "chunk.x must be a non-negative region-local tile offset"
        Assert-True ([int64]$Chunk.y -ge 0) "chunk.y must be a non-negative region-local tile offset"

        foreach ($Tile in $Chunk.tiles) {
            $TotalTiles++
            Assert-True ([int64]$Tile.x -ge 0) 'tile.x must be non-negative'
            Assert-True ([int64]$Tile.y -ge 0) 'tile.y must be non-negative'

            $RegionLocalX = [int64]$Chunk.x + [int64]$Tile.x
            $RegionLocalY = [int64]$Chunk.y + [int64]$Tile.y
            $GlobalX = $RegionMinX + $RegionLocalX
            $GlobalY = $RegionMinY + $RegionLocalY
            $GlobalZ = [int64]$Chunk.floor

            Assert-True ($GlobalX -ge $RegionMinX -and $GlobalX -lt $RegionMaxX) "tile resolves outside region X bounds: $GlobalX"
            Assert-True ($GlobalY -ge $RegionMinY -and $GlobalY -lt $RegionMaxY) "tile resolves outside region Y bounds: $GlobalY"

            $Key = "$GlobalZ:$GlobalX:$GlobalY"
            Assert-True (-not $Resolved.ContainsKey($Key)) "two chunks resolve to the same tile coordinate: $Key"
            $Resolved[$Key] = [pscustomobject]@{
                ChunkX = [int64]$Chunk.x
                ChunkY = [int64]$Chunk.y
                TileX = [int64]$Tile.x
                TileY = [int64]$Tile.y
                Ground = [string]$Tile.ground
            }

            if (@($Tile.tags) -contains 'development-spawn') {
                if ($GlobalX -eq [int64]$Map.developmentSpawn.x -and
                    $GlobalY -eq [int64]$Map.developmentSpawn.y -and
                    $GlobalZ -eq [int64]$Map.developmentSpawn.z) {
                    $SpawnMatches++
                }
            }
        }
    }

    foreach ($Entry in $Resolved.GetEnumerator()) {
        $Parts = $Entry.Key.Split(':')
        $Z = [int64]$Parts[0]
        $X = [int64]$Parts[1]
        $Y = [int64]$Parts[2]
        $RightKey = "$Z:$($X + 1):$Y"
        if ($Resolved.ContainsKey($RightKey)) {
            $A = $Entry.Value
            $B = $Resolved[$RightKey]
            if ($A.ChunkX -ne $B.ChunkX -or $A.ChunkY -ne $B.ChunkY) {
                $CrossChunkPairs++
            }
        }
    }
}

Assert-True ($TotalChunks -ge 4) "fixture must exercise at least four chunks; found $TotalChunks"
Assert-True ($TotalTiles -ge 10) "fixture must contain enough tiles for editor testing; found $TotalTiles"
Assert-True ($SpawnMatches -eq 1) "development spawn must resolve exactly once; found $SpawnMatches"
Assert-True ($CrossChunkPairs -ge 1) 'fixture must contain at least one horizontally adjacent pair across a chunk boundary'

Write-Host "FMAP multi-chunk validation PASS. chunks=$TotalChunks tiles=$TotalTiles crossChunkPairs=$CrossChunkPairs"
