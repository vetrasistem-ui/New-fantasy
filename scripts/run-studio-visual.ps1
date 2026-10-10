param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [switch]$Build
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root 'build/studio'

Push-Location $Root
try {
    $Candidates = @(
        (Join-Path $BuildDir "$Configuration/FantasyStudio.exe"),
        (Join-Path $BuildDir 'FantasyStudio.exe'),
        # Backward-compatible fallback for old build directories.
        (Join-Path $BuildDir "$Configuration/fantasy-studio-gui.exe"),
        (Join-Path $BuildDir 'fantasy-studio-gui.exe')
    )

    $Gui = $Candidates | Where-Object { Test-Path $_ } | Select-Object -First 1

    if ($Build -or -not $Gui) {
        Write-Host "Building Fantasy Studio ($Configuration)..."
        cmake -S Studio -B build/studio
        if ($LASTEXITCODE -ne 0) { throw "Fantasy Studio configure failed. exit=$LASTEXITCODE" }

        cmake --build build/studio --config $Configuration
        if ($LASTEXITCODE -ne 0) { throw "Fantasy Studio build failed. exit=$LASTEXITCODE" }

        $Gui = $Candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
    }

    if (-not $Gui) {
        throw 'FantasyStudio.exe was not found after build.'
    }

    Write-Host "Launching Fantasy Studio from: $Gui"
    & $Gui $Root

    if ($LASTEXITCODE -ne 0) {
        throw "Fantasy Studio exited with code $LASTEXITCODE"
    }

    $Orphans = @(
        Get-Process FantasyStudio -ErrorAction SilentlyContinue
        Get-Process fantasy-studio-gui -ErrorAction SilentlyContinue
    ) | Where-Object { $_ }
    if ($Orphans) {
        throw 'Fantasy Studio left an orphan process after close.'
    }

    Write-Host 'Fantasy Studio launch/close gate PASS.'
}
finally {
    Pop-Location
}
