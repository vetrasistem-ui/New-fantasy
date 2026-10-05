$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$UpstreamRoot = Join-Path $Root '.upstream'
New-Item -ItemType Directory -Force -Path $UpstreamRoot | Out-Null

function Get-PinnedReference {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Url,
        [Parameter(Mandatory = $true)][string]$Sha
    )

    $Target = Join-Path $UpstreamRoot $Name

    if (-not (Test-Path (Join-Path $Target '.git'))) {
        Write-Host "[clone-reference] $Name"
        git clone --no-checkout $Url $Target
        if ($LASTEXITCODE -ne 0) { throw "Failed to clone reference $Name" }
    }

    Push-Location $Target
    try {
        Write-Host "[fetch-reference] $Name"
        git fetch --all --tags
        if ($LASTEXITCODE -ne 0) { throw "Failed to fetch reference $Name" }

        Write-Host "[checkout-reference] $Name @ $Sha"
        git checkout --detach $Sha
        if ($LASTEXITCODE -ne 0) { throw "Failed to checkout $Name @ $Sha" }

        $Actual = (git rev-parse HEAD).Trim()
        if ($Actual -ne $Sha) {
            throw "SHA mismatch for $Name. Expected $Sha, got $Actual"
        }

        Write-Host "[ok-reference] $Name @ $Actual"
    }
    finally {
        Pop-Location
    }
}

Get-PinnedReference -Name 'tfs-1.4.2' `
    -Url 'https://github.com/otland/forgottenserver.git' `
    -Sha '31d6e85de2a86fb3f0e36c63509fba75b855b8bd'

Get-PinnedReference -Name 'rme-3.7' `
    -Url 'https://github.com/hampusborgos/rme.git' `
    -Sha '6aceb3c6a311e6e1c0b24a0bf06cf383fb152766'

Get-PinnedReference -Name 'otclient-10.98-reference' `
    -Url 'https://github.com/opentibiabr/otclient.git' `
    -Sha '396f0b396741bdd4469f27cf9376103930712cff'

Write-Host ''
Write-Host 'Pinned reference implementations are ready under .upstream/'
Write-Host 'They are behavioral oracles/fallbacks only; do not copy them into the native Fantasy core without an explicit ADR and license review.'
