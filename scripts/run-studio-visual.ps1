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
        throw 'fantasy-studio-gui.exe was not found after build.'
    }

    Write-Host "Launching Fantasy Studio visual target from: $Gui"
    Write-Host 'Check Home, Sidebar, Topbar, Map and Items & Assets against docs/STUDIO-VISUAL-TARGET-V1.md.'
    & $Gui $Root

    if ($LASTEXITCODE -ne 0) {
        throw "Fantasy Studio GUI exited with code $LASTEXITCODE"
    }

    $Orphan = Get-Process fantasy-studio-gui -ErrorAction SilentlyContinue
    if ($Orphan) {
        throw 'Fantasy Studio GUI left an orphan process after close.'
    }

    Write-Host 'Fantasy Studio visual launch/close gate PASS.'
}
finally {
    Pop-Location
}
