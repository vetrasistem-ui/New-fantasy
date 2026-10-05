param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [int]$Port = 7171,
    [string]$Character = 'Development Hero'
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Server = Join-Path $Root "build/server/$Configuration/fantasy-server.exe"
$Client = Join-Path $Root "build/client/$Configuration/fantasy-client.exe"

if (-not (Test-Path $Server)) { throw "Fantasy Server not found: $Server" }
if (-not (Test-Path $Client)) { throw "Fantasy Client not found: $Client" }
if ($Port -lt 1 -or $Port -gt 65535) { throw 'Port must be between 1 and 65535.' }

$ServerProcess = $null
try {
    Write-Host "Starting Fantasy Server on 127.0.0.1:$Port ..."
    $ServerProcess = Start-Process -FilePath $Server -ArgumentList @('--serve-once', "$Port") -PassThru -NoNewWindow

    $Connected = $false
    for ($Attempt = 1; $Attempt -le 20; $Attempt++) {
        Start-Sleep -Milliseconds 250
        & $Client --connect 127.0.0.1 $Port $Character
        $ClientExit = $LASTEXITCODE
        if ($ClientExit -eq 0) {
            $Connected = $true
            break
        }
        if ($ServerProcess.HasExited) {
            throw "Fantasy Server exited before a successful client session. exit=$($ServerProcess.ExitCode)"
        }
        Write-Host "Client connect attempt $Attempt failed; retrying..."
    }

    if (-not $Connected) { throw 'Fantasy Client could not complete native play session.' }

    if (-not $ServerProcess.WaitForExit(10000)) {
        throw 'Fantasy Server did not stop after serve-once session.'
    }
    if ($ServerProcess.ExitCode -ne 0) {
        throw "Fantasy Server serve-once failed. exit=$($ServerProcess.ExitCode)"
    }

    Write-Host 'Two-process native play PASS.'
}
finally {
    if ($null -ne $ServerProcess -and -not $ServerProcess.HasExited) {
        Stop-Process -Id $ServerProcess.Id -Force -ErrorAction SilentlyContinue
    }
}
