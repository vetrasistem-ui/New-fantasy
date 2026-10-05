$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$TempBase = if ($env:RUNNER_TEMP) { $env:RUNNER_TEMP } else { [System.IO.Path]::GetTempPath() }
$RelocatedRoot = Join-Path $TempBase ("fantasy-relocated-" + [guid]::NewGuid().ToString('N'))

Write-Host "Relocation test source: $Root"
Write-Host "Relocation test target: $RelocatedRoot"
New-Item -ItemType Directory -Force -Path $RelocatedRoot | Out-Null

try {
    Push-Location $Root
    try {
        $TrackedFiles = @(git ls-files)
        if ($LASTEXITCODE -ne 0) { throw 'git ls-files failed' }
    }
    finally {
        Pop-Location
    }

    if ($TrackedFiles.Count -eq 0) { throw 'No tracked files found for relocation test' }

    foreach ($Relative in $TrackedFiles) {
        $Source = Join-Path $Root $Relative
        $Target = Join-Path $RelocatedRoot $Relative
        $TargetDirectory = Split-Path -Parent $Target
        if ($TargetDirectory) {
            New-Item -ItemType Directory -Force -Path $TargetDirectory | Out-Null
        }
        Copy-Item -LiteralPath $Source -Destination $Target -Force
    }

    & (Join-Path $RelocatedRoot 'scripts/check-layout.ps1')
    if ($LASTEXITCODE -ne 0) { throw 'Relocated layout validation failed' }

    & (Join-Path $RelocatedRoot 'scripts/validate-project.ps1')
    if ($LASTEXITCODE -ne 0) { throw 'Relocated project validation failed' }

    & (Join-Path $RelocatedRoot 'scripts/validate-fmap.ps1')
    if ($LASTEXITCODE -ne 0) { throw 'Relocated FMAP validation failed' }

    & (Join-Path $RelocatedRoot 'scripts/validate-protocol.ps1')
    if ($LASTEXITCODE -ne 0) { throw 'Relocated protocol validation failed' }

    Write-Host 'Project relocation PASS: all tracked project contracts work from a different root.'
}
finally {
    if (Test-Path $RelocatedRoot) {
        Remove-Item -LiteralPath $RelocatedRoot -Recurse -Force
    }
}
