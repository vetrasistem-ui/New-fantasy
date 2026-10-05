param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [switch]$SkipStudio
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
Push-Location $Root

try {
    Write-Host '== Fantasy contracts =='
    & "$PSScriptRoot/check-layout.ps1"
    & "$PSScriptRoot/validate-project.ps1"
    & "$PSScriptRoot/validate-fmap.ps1"
    & "$PSScriptRoot/validate-fmap-multichunk.ps1"
    & "$PSScriptRoot/validate-protocol.ps1"
    & "$PSScriptRoot/test-project-relocation.ps1"

    if (-not $SkipStudio) {
        Write-Host '== Fantasy Studio =='
        cmake -S Studio -B build/studio
        cmake --build build/studio --config $Configuration
        ctest --test-dir build/studio -C $Configuration --output-on-failure
    }

    Write-Host '== Fantasy Server =='
    cmake -S Server -B build/server
    cmake --build build/server --config $Configuration
    ctest --test-dir build/server -C $Configuration --output-on-failure

    Write-Host '== Fantasy Client =='
    cmake -S Client -B build/client
    cmake --build build/client --config $Configuration
    ctest --test-dir build/client -C $Configuration --output-on-failure

    Write-Host 'Fantasy native build PASS.'
}
finally {
    Pop-Location
}
