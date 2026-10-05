param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [int]$Port = 7171,
    [string]$Character = 'Development Hero'
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Server = Join-Path $Root "build/server/$Configuration/fantasy-server.exe"
$ClientGui = Join-Path $Root "build/client/$Configuration/fantasy-client-gui.exe"

if (-not (Test-Path $Server)) { throw "Fantasy Server not found: $Server" }
if (-not (Test-Path $ClientGui)) { throw "Fantasy Client GUI not found: $ClientGui" }
if ($Port -lt 1 -or $Port -gt 65535) { throw 'Port must be between 1 and 65535.' }

$ServerProcess = $null
try {
    Write-Host "Starting Fantasy Server on 127.0.0.1:$Port ..."
    $ServerProcess = Start-Process -FilePath $Server -ArgumentList @('--serve-once', "$Port") -PassThru -NoNewWindow
    Start-Sleep -Milliseconds 750

    if ($ServerProcess.HasExited) {
        throw "Fantasy Server exited before visual Client launch. exit=$($ServerProcess.ExitCode)"
    }

    Write-Host 'Launching Fantasy Client GUI.'
    Write-Host 'Validate: four chunks/64 tiles visible, player at spawn, arrow-key movement, then close the window.'
    & $ClientGui --connect 127.0.0.1 $Port $Character
    if ($LASTEXITCODE -ne 0) {
        throw "Fantasy Client GUI failed. exit=$LASTEXITCODE"
    }

    if (-not $ServerProcess.WaitForExit(10000)) {
        throw 'Fantasy Server did not stop after visual Client disconnect.'
    }
    if ($ServerProcess.ExitCode -ne 0) {
        throw "Fantasy Server visual session failed. exit=$($ServerProcess.ExitCode)"
    }

    Write-Host 'Visual native-play process gate PASS.'
}
finally {
    if ($null -ne $ServerProcess -and -not $ServerProcess.HasExited) {
        Stop-Process -Id $ServerProcess.Id -Force -ErrorAction SilentlyContinue
    }
}
