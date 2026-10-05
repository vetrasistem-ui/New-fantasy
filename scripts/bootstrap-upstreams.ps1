$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$UpstreamRoot = Join-Path $Root '.upstream'
New-Item -ItemType Directory -Force -Path $UpstreamRoot | Out-Null

function Get-PinnedRepo {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Url,
        [Parameter(Mandatory = $true)][string]$Sha
    )

    $Target = Join-Path $UpstreamRoot $Name

    if (-not (Test-Path (Join-Path $Target '.git'))) {
        Write-Host "[clone] $Name"
        git clone --no-checkout $Url $Target
        if ($LASTEXITCODE -ne 0) { throw "Failed to clone $Name" }
    }

    Push-Location $Target
    try {
        Write-Host "[fetch] $Name"
        git fetch --all --tags
        if ($LASTEXITCODE -ne 0) { throw "Failed to fetch $Name" }

        Write-Host "[checkout] $Name @ $Sha"
        git checkout --detach $Sha
        if ($LASTEXITCODE -ne 0) { throw "Failed to checkout $Name @ $Sha" }

        $Actual = (git rev-parse HEAD).Trim()
        if ($Actual -ne $Sha) {
            throw "SHA mismatch for $Name. Expected $Sha, got $Actual"
        }

        Write-Host "[ok] $Name @ $Actual"
    }
    finally {
        Pop-Location
    }
}

Get-PinnedRepo -Name 'tfs-1.4.2' `
    -Url 'https://github.com/otland/forgottenserver.git' `
    -Sha '31d6e85de2a86fb3f0e36c63509fba75b855b8bd'

Get-PinnedRepo -Name 'rme-3.7' `
    -Url 'https://github.com/hampusborgos/rme.git' `
    -Sha '6aceb3c6a311e6e1c0b24a0bf06cf383fb152766'

Get-PinnedRepo -Name 'otclient' `
    -Url 'https://github.com/opentibiabr/otclient.git' `
    -Sha '396f0b396741bdd4469f27cf9376103930712cff'

Write-Host ''
Write-Host 'Pinned upstream candidates are ready under .upstream/'
Write-Host 'Do not copy them into the product before F00 compatibility and license gates pass.'
